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

#ifndef PDF_PRIM_H
#define PDF_PRIM_H

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


typedef struct pdf_obj *pdf_obj_handle;


typedef void (*pdf_stream_write_callback)(pdf_file_handle pdf_file,
					  pdf_obj_handle stream,
					  void *app_data);


/* returns -1 if o1 < 02, 0 if o1 == o2, 1 if o1 > o2 */
/* only works for integer, real, string, and name objects */
int pdf_compare_obj (pdf_obj_handle o1, pdf_obj_handle o2);


void pdf_set_dict_entry (pdf_obj_handle dict_obj, char *key, pdf_obj_handle val);
pdf_obj_handle pdf_get_dict_entry (pdf_obj_handle dict_obj, char *key);


void pdf_add_array_elem (pdf_obj_handle array_obj, pdf_obj_handle val);


/* Following is intended for things like ProcSet in which an array object
   is used to represent a set.  Only works if all objects in array, and
   the element to be added are of scalar types (types that are supported
   by pdf_compare_obj.  Not efficient for large arrays as it does a
   comaprison to every element. */
void pdf_add_array_elem_unique (pdf_obj_handle array_obj, pdf_obj_handle val);


/* Create a new object that will NOT be used indirectly */
pdf_obj_handle pdf_new_obj (pdf_obj_type type);

pdf_obj_handle pdf_new_bool (bool val);

pdf_obj_handle pdf_new_name (char *name);

pdf_obj_handle pdf_new_string (char *str);

pdf_obj_handle pdf_new_string_n (char *str, int n);

pdf_obj_handle pdf_new_integer (long val);

pdf_obj_handle pdf_new_real (double val);


/* Create a new indirect object */
pdf_obj_handle pdf_new_ind_ref (pdf_file_handle pdf_file, pdf_obj_handle obj);

/* get the object referenced by an indirect reference */
pdf_obj_handle pdf_deref_ind_obj (pdf_obj_handle ind_obj);


long pdf_get_integer (pdf_obj_handle obj);
void pdf_set_integer (pdf_obj_handle obj, long val);


double pdf_get_real (pdf_obj_handle obj);
void pdf_set_real (pdf_obj_handle obj, double val);


/* The callback will be called when the stream data is to be written to the
   file.  app_data will be passed as an argument to the callback. */
pdf_obj_handle pdf_new_stream (pdf_file_handle pdf_file,
				pdf_obj_handle stream_dict,
				pdf_stream_write_callback callback,
				void *app_data);

/* The callback should call pdf_stream_write_data() or pdf_stream_printf()
   to write the actual stream data. */

void pdf_stream_flush_bits (pdf_file_handle pdf_file,
			    pdf_obj_handle stream);

void pdf_stream_write_data (pdf_file_handle pdf_file,
			    pdf_obj_handle stream,
			    char *data,
			    unsigned long len);

void pdf_stream_printf (pdf_file_handle pdf_file,
			pdf_obj_handle stream,
			char *fmt, ...);


void pdf_stream_add_filter (pdf_obj_handle stream,
			    char *filter_name,
			    pdf_obj_handle decode_parms);


/* Write the object to the file */
void pdf_write_obj (pdf_file_handle pdf_file, pdf_obj_handle obj);


/* Write the indirect object to the file.  For most objects this should
   be done by pdf_write_all_ind_obj() when the file is being closed, but for
   large objects such as streams, it's probably better to do it as soon as the
   object is complete. */
void pdf_write_ind_obj (pdf_file_handle pdf_file, pdf_obj_handle ind_obj);


/* Write all indirect objects that haven't already been written to the file. */
void pdf_write_all_ind_obj (pdf_file_handle pdf_file);


/* Write the cross reference table, and return the maximum object number */
unsigned long pdf_write_xref (pdf_file_handle pdf_file);


/* Write a name, escaping reserved characters */
void pdf_write_name (pdf_file_handle pdf_file, char *s);


/* this isn't really a PDF primitive data type */
char pdf_new_XObject (pdf_page_handle pdf_page, pdf_obj_handle ind_ref);


void pdf_page_add_content_stream(pdf_page_handle pdf_page,
				 pdf_obj_handle content_stream);


#endif // PDF_PRIM_H
