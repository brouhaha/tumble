#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdf.h"
#include "pdf_util.h"
#include "pdf_prim.h"
#include "pdf_private.h"


struct pdf_array_elem
{
  struct pdf_array_elem *next;
  struct pdf_obj        *val;
};


struct pdf_array
{
  struct pdf_array_elem *first;
  struct pdf_array_elem *last;
};


struct pdf_dict_entry
{
  struct pdf_dict_entry *next;
  char                  *key;
  struct pdf_obj        *val;
};


struct pdf_dict
{
  struct pdf_dict_entry *first;
};


struct pdf_stream
{
  struct pdf_obj *stream_dict;
  struct pdf_obj *length;
  pdf_stream_write_callback callback;
  void *app_data;  /* arg to pass to callback */
  struct pdf_obj *filters;  /* name or array of names */
  struct pdf_obj *decode_parms;
};


struct pdf_obj
{
  /* these fields only apply to indirectly referenced objects */
  struct pdf_obj      *prev;
  struct pdf_obj      *next;
  unsigned long       obj_num;
  unsigned long       obj_gen;
  long int            file_offset;

  /* these fields apply to all objects */
  unsigned long       ref_count;
  pdf_obj_type        type;
  union {
    int               bool;
    char              *name;
    char              *string;
    unsigned long     integer;
    double            real;
    struct pdf_obj    *ind_ref;
    struct pdf_dict   dict;
    struct pdf_array  array;
    struct pdf_stream stream;
  } val;
};


struct pdf_obj *ref (struct pdf_obj *obj)
{
  obj->ref_count++;
  return (obj);
}


void unref (struct pdf_obj *obj)
{
  if ((--obj->ref_count) == 0)
    {
      /* $$$ free the object */
    }
}


struct pdf_obj *pdf_deref_ind_obj (struct pdf_obj *ind_obj)
{
  pdf_assert (ind_obj->type == PT_IND_REF);
  return (ind_obj->val.ind_ref);
}


void pdf_set_dict_entry (struct pdf_obj *dict_obj, char *key, struct pdf_obj *val)
{
  struct pdf_dict_entry *entry;

  if (dict_obj->type == PT_IND_REF)
    dict_obj = pdf_deref_ind_obj (dict_obj);

  pdf_assert (dict_obj->type == PT_DICTIONARY);

  /* replacing existing entry? */
  for (entry = dict_obj->val.dict.first; entry; entry = entry->next)
    if (strcmp (entry->key, key) == 0)
      {
	unref (entry->val);
	entry->val = ref (val);
	return;
      }

  /* new entry */
  entry = pdf_calloc (sizeof (struct pdf_dict_entry));

  entry->next = dict_obj->val.dict.first;
  dict_obj->val.dict.first = entry;

  entry->key = pdf_strdup (key);
  entry->val = ref (val);
}


struct pdf_obj *pdf_get_dict_entry (struct pdf_obj *dict_obj, char *key)
{
  struct pdf_dict_entry *entry;

  if (dict_obj->type == PT_IND_REF)
    dict_obj = pdf_deref_ind_obj (dict_obj);

  pdf_assert (dict_obj->type == PT_DICTIONARY);

  for (entry = dict_obj->val.dict.first; entry; entry = entry->next)
    if (strcmp (entry->key, key) == 0)
      return (entry->val);

  return (NULL);
}


void pdf_add_array_elem (struct pdf_obj *array_obj, struct pdf_obj *val)
{
  struct pdf_array_elem *elem = pdf_calloc (sizeof (struct pdf_array_elem));

  if (array_obj->type == PT_IND_REF)
    array_obj = pdf_deref_ind_obj (array_obj);

  pdf_assert (array_obj->type == PT_ARRAY);

  elem->val = ref (val);

  if (! array_obj->val.array.first)
    array_obj->val.array.first = elem;
  else
    array_obj->val.array.last->next = elem;

  array_obj->val.array.last = elem;
}


struct pdf_obj *pdf_new_obj (pdf_obj_type type)
{
  struct pdf_obj *obj = pdf_calloc (sizeof (struct pdf_obj));
  obj->type = type;
  return (obj);
}


struct pdf_obj *pdf_new_bool (int bool)
{
  struct pdf_obj *obj = pdf_new_obj (PT_BOOL);
  obj->val.bool = bool;
  return (obj);
}


struct pdf_obj *pdf_new_name (char *name)
{
  struct pdf_obj *obj = pdf_new_obj (PT_NAME);
  obj->val.name = pdf_strdup (name);
  return (obj);
}


struct pdf_obj *pdf_new_string (char *str)
{
  struct pdf_obj *obj = pdf_new_obj (PT_STRING);
  obj->val.string = pdf_strdup (str);
  return (obj);
}


struct pdf_obj *pdf_new_integer (unsigned long val)
{
  struct pdf_obj *obj = pdf_new_obj (PT_INTEGER);
  obj->val.integer = val;
  return (obj);
}


struct pdf_obj *pdf_new_real (double val)
{
  struct pdf_obj *obj = pdf_new_obj (PT_REAL);
  obj->val.real = val;
  return (obj);
}


struct pdf_obj *pdf_new_stream (pdf_file_handle pdf_file,
				struct pdf_obj *stream_dict,
				pdf_stream_write_callback callback,
				void *app_data)
{
  struct pdf_obj *obj = pdf_new_obj (PT_STREAM);

  obj->val.stream.stream_dict = stream_dict;
  obj->val.stream.length = pdf_new_ind_ref (pdf_file, pdf_new_integer (0));
  pdf_set_dict_entry (obj->val.stream.stream_dict, "Length", obj->val.stream.length);

  obj->val.stream.callback = callback;
  obj->val.stream.app_data = app_data;
  return (obj);
}


/* $$$ currently limited to one filter per stream */
void pdf_stream_add_filter (struct pdf_obj *stream,
			    char *filter_name,
			    struct pdf_obj *decode_parms)
{
  if (stream->type == PT_IND_REF)
    stream = pdf_deref_ind_obj (stream);

  pdf_assert (stream->type == PT_STREAM);

  pdf_set_dict_entry (stream->val.stream.stream_dict, "Filter", pdf_new_name (filter_name));
  if (decode_parms)
    pdf_set_dict_entry (stream->val.stream.stream_dict, "DecodeParms", decode_parms);
}


struct pdf_obj *pdf_new_ind_ref (pdf_file_handle pdf_file, struct pdf_obj *obj)
{
  struct pdf_obj *ind_obj;

  pdf_assert (obj->type != PT_IND_REF);

  ind_obj = pdf_new_obj (PT_IND_REF);

  ind_obj->type = PT_IND_REF;
  ind_obj->val.ind_ref = obj;

  /* is there already an indirect reference to this object? */
  if (! obj->obj_num)
    {
      /* no, assign object number/generation and add to linked list */
      if (! pdf_file->first_ind_obj)
	{
	  obj->obj_num = 1;
	  pdf_file->first_ind_obj = pdf_file->last_ind_obj = obj;
	}
      else
	{
	  obj->obj_num = pdf_file->last_ind_obj->obj_num + 1;
	  pdf_file->last_ind_obj->next = obj;
	  obj->prev = pdf_file->last_ind_obj;
	  pdf_file->last_ind_obj = obj;
	}
    }

  return (ind_obj);
}


unsigned long pdf_get_integer (struct pdf_obj *obj)
{
  if (obj->type == PT_IND_REF)
    obj = pdf_deref_ind_obj (obj);

  pdf_assert (obj->type == PT_INTEGER);

  return (obj->val.integer);
}

void pdf_set_integer (struct pdf_obj *obj, unsigned long val)
{
  if (obj->type == PT_IND_REF)
    obj = pdf_deref_ind_obj (obj);

  pdf_assert (obj->type == PT_INTEGER);

  obj->val.integer = val;
}


double pdf_get_real (struct pdf_obj *obj)
{
  if (obj->type == PT_IND_REF)
    obj = pdf_deref_ind_obj (obj);

  pdf_assert (obj->type == PT_REAL);

  return (obj->val.real);
}

void pdf_set_real (struct pdf_obj *obj, double val)
{
  if (obj->type == PT_IND_REF)
    obj = pdf_deref_ind_obj (obj);

  pdf_assert (obj->type == PT_REAL);

  obj->val.real = val;
}


static int name_char_needs_quoting (char c)
{
  return ((c < '!')  || (c > '~')  || (c == '/') || (c == '\\') ||
	  (c == '(') || (c == ')') || (c == '<') || (c == '>')  ||
	  (c == '[') || (c == ']') || (c == '{') || (c == '}')  ||
	  (c == '%'));
}


void pdf_write_name (pdf_file_handle pdf_file, char *s)
{
  fprintf (pdf_file->f, "/");
  while (*s)
    if (name_char_needs_quoting (*s))
      fprintf (pdf_file->f, "#%02x", 0xff & *(s++));
    else
      fprintf (pdf_file->f, "%c", *(s++));
  fprintf (pdf_file->f, " ");
}


static int string_char_needs_quoting (char c)
{
  return ((c < ' ')  || (c > '~')  || (c == '\\') ||
	  (c == '(') || (c == ')'));
}


void pdf_write_string (pdf_file_handle pdf_file, char *s)
{
  fprintf (pdf_file->f, "(");
  while (*s)
    if (string_char_needs_quoting (*s))
      fprintf (pdf_file->f, "\\%03o", 0xff & *(s++));
    else
      fprintf (pdf_file->f, "%c", *(s++));
  fprintf (pdf_file->f, ") ");
}


void pdf_write_real (pdf_file_handle pdf_file, double num)
{
  /* $$$ not actually good enough, precision needs to be variable,
     and no exponent is allowed */
  fprintf (pdf_file->f, "%0f ", num);
}


void pdf_write_ind_ref (pdf_file_handle pdf_file, struct pdf_obj *ind_obj)
{
  struct pdf_obj *obj = pdf_deref_ind_obj (ind_obj);
  fprintf (pdf_file->f, "%ld %ld R ", obj->obj_num, obj->obj_gen);
}


void pdf_write_array (pdf_file_handle pdf_file, struct pdf_obj *array_obj)
{
  struct pdf_array_elem *elem;

  pdf_assert (array_obj->type == PT_ARRAY);

  fprintf (pdf_file->f, "[ ");
  for (elem = array_obj->val.array.first; elem; elem = elem->next)
    {
      pdf_write_obj (pdf_file, elem->val);
      fprintf (pdf_file->f, " ");
    }
  fprintf (pdf_file->f, "] ");
}


void pdf_write_dict (pdf_file_handle pdf_file, struct pdf_obj *dict_obj)
{
  struct pdf_dict_entry *entry;

  pdf_assert (dict_obj->type == PT_DICTIONARY);

  fprintf (pdf_file->f, "<<\r\n");
  for (entry = dict_obj->val.dict.first; entry; entry = entry->next)
    {
      pdf_write_name (pdf_file, entry->key);
      fprintf (pdf_file->f, " ");
      pdf_write_obj (pdf_file, entry->val);
      fprintf (pdf_file->f, "\r\n");
    }
  fprintf (pdf_file->f, ">>\r\n");
}


void pdf_stream_write_data (pdf_file_handle pdf_file,
			    struct pdf_obj *stream,
			    char *data,
			    unsigned long len)
{
  while (len)
    {
      unsigned long l2 = fwrite (data, 1, len, pdf_file->f);
      data += l2;
      len -= l2;
      if (ferror (pdf_file->f))
	pdf_fatal ("error writing stream data\n");
    }
}


void pdf_write_stream (pdf_file_handle pdf_file, struct pdf_obj *stream)
{
  unsigned long begin_pos, end_pos;

  pdf_assert (stream->type == PT_STREAM);

  pdf_write_dict (pdf_file, stream->val.stream.stream_dict);
  fprintf (pdf_file->f, "stream\r\n");
  begin_pos = ftell (pdf_file->f);
  stream->val.stream.callback (pdf_file,
			       stream,
			       stream->val.stream.app_data);
  end_pos = ftell (pdf_file->f);
  fprintf (pdf_file->f, "\r\nendstream\r\n");

  pdf_set_integer (stream->val.stream.length, end_pos - begin_pos);
}


void pdf_write_obj (pdf_file_handle pdf_file, struct pdf_obj *obj)
{
  switch (obj->type)
    {
    case PT_NULL:
      fprintf (pdf_file->f, "null ");
      break;
    case PT_BOOL:
      if (obj->val.bool)
	fprintf (pdf_file->f, "true ");
      else
	fprintf (pdf_file->f, "false ");
      break;
    case PT_NAME:
      pdf_write_name (pdf_file, obj->val.name);
      break;
    case PT_STRING:
      pdf_write_string (pdf_file, obj->val.string);
      break;
    case PT_INTEGER:
      fprintf (pdf_file->f, "%ld ", obj->val.integer);
      break;
    case PT_REAL:
      pdf_write_real (pdf_file, obj->val.real);
      break;
    case PT_IND_REF:
      pdf_write_ind_ref (pdf_file, obj);
      break;
    case PT_DICTIONARY:
      pdf_write_dict (pdf_file, obj);
      break;
    case PT_ARRAY:
      pdf_write_array (pdf_file, obj);
      break;
    case PT_STREAM:
      pdf_write_stream (pdf_file, obj);
      break;
    default:
      pdf_fatal ("bad object type\n");
    }
}


void pdf_write_ind_obj (pdf_file_handle pdf_file, struct pdf_obj *ind_obj)
{
  struct pdf_obj *obj;

  if (ind_obj->type == PT_IND_REF)
    obj = pdf_deref_ind_obj (ind_obj);
  else
    obj = ind_obj;

  obj->file_offset = ftell (pdf_file->f);
  fprintf (pdf_file->f, "%ld %ld obj\r\n", obj->obj_num, obj->obj_gen);
  pdf_write_obj (pdf_file, obj);
  fprintf (pdf_file->f, "endobj\r\n");
}


void pdf_write_all_ind_obj (pdf_file_handle pdf_file)
{
  struct pdf_obj *ind_obj;
  for (ind_obj = pdf_file->first_ind_obj; ind_obj; ind_obj = ind_obj->next)
    if (! ind_obj->file_offset)
      pdf_write_ind_obj (pdf_file, ind_obj);
}


unsigned long pdf_write_xref (pdf_file_handle pdf_file)
{
  struct pdf_obj *ind_obj;
  pdf_file->xref_offset = ftell (pdf_file->f);
  fprintf (pdf_file->f, "xref\r\n");
  fprintf (pdf_file->f, "0 %ld\r\n", pdf_file->last_ind_obj->obj_num + 1);
  fprintf (pdf_file->f, "0000000000 65535 f\r\n");
  for (ind_obj = pdf_file->first_ind_obj; ind_obj; ind_obj = ind_obj->next)
    fprintf (pdf_file->f, "%010ld 00000 n\r\n", ind_obj->file_offset);
  return (pdf_file->last_ind_obj->obj_num + 1);
}


