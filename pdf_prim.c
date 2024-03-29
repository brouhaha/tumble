/*
 * tumble: build a PDF file from image files
 *
 * PDF routines
 * Copyright 2001, 2002, 2003, 2017 Eric Smith <spacewar@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.  Note that permission is
 * not granted to redistribute this program under the terms of any
 * other version of the General Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 *
 *  2007-05-07 [JDB] Allow embedded nulls in strings by storing as character
 *                   arrays plus length words.
 */


#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitblt.h"
#include "pdf.h"
#include "pdf_util.h"
#include "pdf_prim.h"
#include "pdf_private.h"


struct pdf_array_elem
{
  struct pdf_array_elem *next;
  pdf_obj_handle        val;
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
  pdf_obj_handle        val;
};


struct pdf_dict
{
  struct pdf_dict_entry *first;
};


struct pdf_stream
{
  pdf_obj_handle stream_dict;
  pdf_obj_handle length;
  pdf_stream_write_callback callback;
  void *app_data;  /* arg to pass to callback */
  pdf_obj_handle filters;  /* name or array of names */
  pdf_obj_handle decode_parms;
};


struct pdf_obj
{
  /* these fields only apply to indirectly referenced objects */
  pdf_obj_handle      prev;
  pdf_obj_handle      next;
  unsigned long       obj_num;
  unsigned long       obj_gen;
  long int            file_offset;

  /* these fields apply to all objects */
  unsigned long       ref_count;
  pdf_obj_type        type;
  union {
    bool              boolean;
    char              *name;
    struct {
      char            *content;
      int             length;
    }                 string;
    long              integer;
    double            real;
    pdf_obj_handle    ind_ref;
    struct pdf_dict   dict;
    struct pdf_array  array;
    struct pdf_stream stream;
  } val;
};


pdf_obj_handle ref (pdf_obj_handle obj)
{
  obj->ref_count++;
  return (obj);
}


void unref (pdf_obj_handle obj)
{
  if ((--obj->ref_count) == 0)
    {
      /* $$$ free the object */
    }
}


pdf_obj_handle pdf_deref_ind_obj (pdf_obj_handle ind_obj)
{
  pdf_assert (ind_obj->type == PT_IND_REF);
  return (ind_obj->val.ind_ref);
}


void pdf_set_dict_entry (pdf_obj_handle dict_obj, char *key, pdf_obj_handle val)
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
  entry = pdf_calloc (1, sizeof (struct pdf_dict_entry));

  entry->next = dict_obj->val.dict.first;
  dict_obj->val.dict.first = entry;

  entry->key = pdf_strdup (key);
  entry->val = ref (val);
}


pdf_obj_handle pdf_get_dict_entry (pdf_obj_handle dict_obj, char *key)
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


void pdf_add_array_elem (pdf_obj_handle array_obj, pdf_obj_handle val)
{
  struct pdf_array_elem *elem = pdf_calloc (1, sizeof (struct pdf_array_elem));

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


void pdf_add_array_elem_unique (pdf_obj_handle array_obj, pdf_obj_handle val)
{
  struct pdf_array_elem *elem;

  if (array_obj->type == PT_IND_REF)
    array_obj = pdf_deref_ind_obj (array_obj);

  pdf_assert (array_obj->type == PT_ARRAY);

  for (elem = array_obj->val.array.first; elem; elem = elem->next)
    if (pdf_compare_obj (val, elem->val) == 0)
      return;

  elem = pdf_calloc (1, sizeof (struct pdf_array_elem));

  elem->val = ref (val);

  if (! array_obj->val.array.first)
    array_obj->val.array.first = elem;
  else
    array_obj->val.array.last->next = elem;

  array_obj->val.array.last = elem;
}


pdf_obj_handle pdf_new_obj (pdf_obj_type type)
{
  pdf_obj_handle obj = pdf_calloc (1, sizeof (struct pdf_obj));
  obj->type = type;
  return (obj);
}


pdf_obj_handle pdf_new_bool (bool val)
{
  pdf_obj_handle obj = pdf_new_obj (PT_BOOL);
  obj->val.boolean = val;
  return (obj);
}


pdf_obj_handle pdf_new_name (char *name)
{
  pdf_obj_handle obj = pdf_new_obj (PT_NAME);
  obj->val.name = pdf_strdup (name);
  return (obj);
}


pdf_obj_handle pdf_new_string (char *str)
{
  pdf_obj_handle obj = pdf_new_obj (PT_STRING);
  obj->val.string.content = pdf_strdup (str);
  obj->val.string.length = strlen(str);
  return (obj);
}


pdf_obj_handle pdf_new_string_n (char *str, int n)
{
  pdf_obj_handle obj = pdf_new_obj (PT_STRING);
  obj->val.string.length = n;
  obj->val.string.content = pdf_calloc (1,n);
  memcpy(obj->val.string.content, str, n);
  return (obj);
}


pdf_obj_handle pdf_new_integer (long val)
{
  pdf_obj_handle obj = pdf_new_obj (PT_INTEGER);
  obj->val.integer = val;
  return (obj);
}


pdf_obj_handle pdf_new_real (double val)
{
  pdf_obj_handle obj = pdf_new_obj (PT_REAL);
  obj->val.real = val;
  return (obj);
}


pdf_obj_handle pdf_new_stream (pdf_file_handle pdf_file,
				pdf_obj_handle stream_dict,
				pdf_stream_write_callback callback,
				void *app_data)
{
  pdf_obj_handle obj = pdf_new_obj (PT_STREAM);

  obj->val.stream.stream_dict = stream_dict;
  obj->val.stream.length = pdf_new_ind_ref (pdf_file, pdf_new_integer (0));
  pdf_set_dict_entry (obj->val.stream.stream_dict, "Length", obj->val.stream.length);

  obj->val.stream.callback = callback;
  obj->val.stream.app_data = app_data;
  return (obj);
}


/* $$$ currently limited to one filter per stream */
void pdf_stream_add_filter (pdf_obj_handle stream,
			    char *filter_name,
			    pdf_obj_handle decode_parms)
{
  if (stream->type == PT_IND_REF)
    stream = pdf_deref_ind_obj (stream);

  pdf_assert (stream->type == PT_STREAM);

  pdf_set_dict_entry (stream->val.stream.stream_dict, "Filter", pdf_new_name (filter_name));
  if (decode_parms)
    pdf_set_dict_entry (stream->val.stream.stream_dict, "DecodeParms", decode_parms);
}


pdf_obj_handle pdf_new_ind_ref (pdf_file_handle pdf_file, pdf_obj_handle obj)
{
  pdf_obj_handle ind_obj;

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


long pdf_get_integer (pdf_obj_handle obj)
{
  if (obj->type == PT_IND_REF)
    obj = pdf_deref_ind_obj (obj);

  pdf_assert (obj->type == PT_INTEGER);

  return (obj->val.integer);
}

void pdf_set_integer (pdf_obj_handle obj, long val)
{
  if (obj->type == PT_IND_REF)
    obj = pdf_deref_ind_obj (obj);

  pdf_assert (obj->type == PT_INTEGER);

  obj->val.integer = val;
}


double pdf_get_real (pdf_obj_handle obj)
{
  if (obj->type == PT_IND_REF)
    obj = pdf_deref_ind_obj (obj);

  pdf_assert (obj->type == PT_REAL);

  return (obj->val.real);
}

void pdf_set_real (pdf_obj_handle obj, double val)
{
  if (obj->type == PT_IND_REF)
    obj = pdf_deref_ind_obj (obj);

  pdf_assert (obj->type == PT_REAL);

  obj->val.real = val;
}


int pdf_compare_obj (pdf_obj_handle o1, pdf_obj_handle o2)
{
  if (o1->type == PT_IND_REF)
    o1 = pdf_deref_ind_obj (o1);

  if (o2->type == PT_IND_REF)
    o2 = pdf_deref_ind_obj (o2);

  pdf_assert (o1->type == o2->type);

  switch (o1->type)
    {
    case PT_INTEGER:
      if (o1->val.integer < o2->val.integer)
	return (-1);
      if (o1->val.integer > o2->val.integer)
	return (1);
      return (0);
    case PT_REAL:
      if (o1->val.real < o2->val.real)
	return (-1);
      if (o1->val.real > o2->val.real)
	return (1);
      return (0);
    case PT_STRING:
      {
	int l;
	l = o1->val.string.length;
	if(l > o2->val.string.length)
	  l = o2->val.string.length;
	l = memcmp (o1->val.string.content, o2->val.string.content, l);
	if (l)
	  return l;
	return o1->val.string.length - o2->val.string.length;
      }
    case PT_NAME:
      return (strcmp (o1->val.name, o2->val.name));
    default:
      pdf_fatal ("invalid object type for comparison\n");
    }
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


static int pdf_write_literal_string (pdf_file_handle pdf_file, char *s, int n)
{
  int i, p;

  if (pdf_file)
    fprintf (pdf_file->f, "(");

  for (i = p = 0; n; n--)
    {
      int j, k;

      k = 0;

      switch (*s)
      {
	case '\\':
	  k = 1;
	  break;

	case '(':
	  for (j = k =1; k && j < n; j++)
	    k += (s[j] == '(') ? 1 : (s[j] == ')') ? -1 : 0;
	  p += !k;
	  break;

	case ')':
	  if (p)
	    p--;
	  else
	    k = 1;
	  break;
      }

      if (k)
	{
	  i++;
	  if (pdf_file)
	    fprintf (pdf_file->f, "\\");
	}

      i++;

      if (pdf_file)
	fprintf (pdf_file->f, "%c", *(s++));
    }

  if (pdf_file)
    fprintf (pdf_file->f, ") ");
  return i;
}


void pdf_write_string (pdf_file_handle pdf_file, char *s, int n)
{
  if (pdf_write_literal_string (NULL, s, n) < 2 * n)
    pdf_write_literal_string (pdf_file, s, n);
  else
    {
      fprintf (pdf_file->f, "<");

      for( ; n--; )
	fprintf (pdf_file->f, "%.2X",*(s++));
      fprintf (pdf_file->f, "> ");
    }
}


void pdf_write_real (pdf_file_handle pdf_file, double num)
{
  /* $$$ not actually good enough, precision needs to be variable,
     and no exponent is allowed */
  fprintf (pdf_file->f, "%0f ", num);
}


void pdf_write_ind_ref (pdf_file_handle pdf_file, pdf_obj_handle ind_obj)
{
  pdf_obj_handle obj = pdf_deref_ind_obj (ind_obj);
  fprintf (pdf_file->f, "%ld %ld R ", obj->obj_num, obj->obj_gen);
}


void pdf_write_array (pdf_file_handle pdf_file, pdf_obj_handle array_obj)
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


void pdf_write_dict (pdf_file_handle pdf_file, pdf_obj_handle dict_obj)
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
			    pdf_obj_handle stream,
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


void pdf_stream_printf (pdf_file_handle pdf_file,
			pdf_obj_handle stream,
			char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (pdf_file->f, fmt, ap);
  va_end (ap);
}


void pdf_write_stream (pdf_file_handle pdf_file, pdf_obj_handle stream)
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


void pdf_write_obj (pdf_file_handle pdf_file, pdf_obj_handle obj)
{
  switch (obj->type)
    {
    case PT_NULL:
      fprintf (pdf_file->f, "null ");
      break;
    case PT_BOOL:
      if (obj->val.boolean)
	fprintf (pdf_file->f, "true ");
      else
	fprintf (pdf_file->f, "false ");
      break;
    case PT_NAME:
      pdf_write_name (pdf_file, obj->val.name);
      break;
    case PT_STRING:
      pdf_write_string (pdf_file, obj->val.string.content, obj->val.string.length);
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


void pdf_write_ind_obj (pdf_file_handle pdf_file, pdf_obj_handle ind_obj)
{
  pdf_obj_handle obj;

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
  pdf_obj_handle ind_obj;
  for (ind_obj = pdf_file->first_ind_obj; ind_obj; ind_obj = ind_obj->next)
    if (! ind_obj->file_offset)
      pdf_write_ind_obj (pdf_file, ind_obj);
}


unsigned long pdf_write_xref (pdf_file_handle pdf_file)
{
  pdf_obj_handle ind_obj;
  pdf_file->xref_offset = ftell (pdf_file->f);
  fprintf (pdf_file->f, "xref\r\n");
  fprintf (pdf_file->f, "0 %ld\r\n", pdf_file->last_ind_obj->obj_num + 1);
  fprintf (pdf_file->f, "0000000000 65535 f\r\n");
  for (ind_obj = pdf_file->first_ind_obj; ind_obj; ind_obj = ind_obj->next)
    fprintf (pdf_file->f, "%010ld 00000 n\r\n", ind_obj->file_offset);
  return (pdf_file->last_ind_obj->obj_num + 1);
}


/* this isn't really a PDF primitive data type */
char pdf_new_XObject (pdf_page_handle pdf_page, pdf_obj_handle ind_ref)
{
  char XObject_name [4] = "Im ";

  XObject_name [2] = ++pdf_page->last_XObject_name;
  
  if (! pdf_page->XObject_dict)
    {
      pdf_page->XObject_dict = pdf_new_obj (PT_DICTIONARY);
      pdf_set_dict_entry (pdf_page->resources, "XObject", pdf_page->XObject_dict);
    }

  pdf_set_dict_entry (pdf_page->XObject_dict, & XObject_name [0], ind_ref);

  return (pdf_page->last_XObject_name);
}


void pdf_page_add_content_stream(pdf_page_handle pdf_page,
				 pdf_obj_handle content_stream)
{
  pdf_obj_handle contents;
  contents = pdf_get_dict_entry (pdf_page->page_dict, "Contents");

  if (! contents)
    contents = pdf_new_obj (PT_ARRAY);
  
  pdf_add_array_elem (contents, content_stream);
  pdf_set_dict_entry (pdf_page->page_dict, "Contents", contents);

  pdf_write_ind_obj (pdf_page->pdf_file, content_stream);
}

