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
 *  2007-05-07 [JDB] Add declarations for pdf_new_string_n() and
 *                   pdf_write_name().
 */


typedef enum
{
  PT_BAD,

  /* scalar */
  PT_NULL,
  PT_BOOL,
  PT_NAME,
  PT_STRING,
  PT_INTEGER,
  PT_REAL,
  PT_IND_REF,

  /* composite */
  PT_DICTIONARY,
  PT_ARRAY,
  PT_STREAM
} pdf_obj_type;


struct pdf_obj;


typedef void (*pdf_stream_write_callback)(pdf_file_handle pdf_file,
					  struct pdf_obj *stream,
					  void *app_data);


/* returns -1 if o1 < 02, 0 if o1 == o2, 1 if o1 > o2 */
/* only works for integer, real, string, and name objects */
int pdf_compare_obj (struct pdf_obj *o1, struct pdf_obj *o2);


void pdf_set_dict_entry (struct pdf_obj *dict_obj, char *key, struct pdf_obj *val);
struct pdf_obj *pdf_get_dict_entry (struct pdf_obj *dict_obj, char *key);


void pdf_add_array_elem (struct pdf_obj *array_obj, struct pdf_obj *val);


/* Following is intended for things like ProcSet in which an array object
   is used to represent a set.  Only works if all objects in array, and
   the element to be added are of scalar types (types that are supported
   by pdf_compare_obj.  Not efficient for large arrays as it does a
   comaprison to every element. */
void pdf_add_array_elem_unique (struct pdf_obj *array_obj, struct pdf_obj *val);


/* Create a new object that will NOT be used indirectly */
struct pdf_obj *pdf_new_obj (pdf_obj_type type);

struct pdf_obj *pdf_new_bool (bool val);

struct pdf_obj *pdf_new_name (char *name);

struct pdf_obj *pdf_new_string (char *str);

struct pdf_obj *pdf_new_string_n (char *str, int n);

struct pdf_obj *pdf_new_integer (long val);

struct pdf_obj *pdf_new_real (double val);


/* Create a new indirect object */
struct pdf_obj *pdf_new_ind_ref (pdf_file_handle pdf_file, struct pdf_obj *obj);

/* get the object referenced by an indirect reference */
struct pdf_obj *pdf_deref_ind_obj (struct pdf_obj *ind_obj);


long pdf_get_integer (struct pdf_obj *obj);
void pdf_set_integer (struct pdf_obj *obj, long val);


double pdf_get_real (struct pdf_obj *obj);
void pdf_set_real (struct pdf_obj *obj, double val);


/* The callback will be called when the stream data is to be written to the
   file.  app_data will be passed as an argument to the callback. */
struct pdf_obj *pdf_new_stream (pdf_file_handle pdf_file,
				struct pdf_obj *stream_dict,
				pdf_stream_write_callback callback,
				void *app_data);

/* The callback should call pdf_stream_write_data() or pdf_stream_printf()
   to write the actual stream data. */

void pdf_stream_flush_bits (pdf_file_handle pdf_file,
			    struct pdf_obj *stream);

void pdf_stream_write_data (pdf_file_handle pdf_file,
			    struct pdf_obj *stream,
			    char *data,
			    unsigned long len);

void pdf_stream_printf (pdf_file_handle pdf_file,
			struct pdf_obj *stream,
			char *fmt, ...);


void pdf_stream_add_filter (struct pdf_obj *stream,
			    char *filter_name,
			    struct pdf_obj *decode_parms);


/* Write the object to the file */
void pdf_write_obj (pdf_file_handle pdf_file, struct pdf_obj *obj);


/* Write the indirect object to the file.  For most objects this should
   be done by pdf_write_all_ind_obj() when the file is being closed, but for
   large objects such as streams, it's probably better to do it as soon as the
   object is complete. */
void pdf_write_ind_obj (pdf_file_handle pdf_file, struct pdf_obj *ind_obj);


/* Write all indirect objects that haven't already been written to the file. */
void pdf_write_all_ind_obj (pdf_file_handle pdf_file);


/* Write the cross reference table, and return the maximum object number */
unsigned long pdf_write_xref (pdf_file_handle pdf_file);


/* Write a name, escaping reserved characters */
void pdf_write_name (pdf_file_handle pdf_file, char *s);


/* this isn't really a PDF primitive data type */
char pdf_new_XObject (pdf_page_handle pdf_page, struct pdf_obj *ind_ref);
