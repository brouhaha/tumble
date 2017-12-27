/*
 * tumble: build a PDF file from image files
 *
 * PDF routines
 * Copyright 2003, 2017 Eric Smith <spacewar@gmail.com>
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
 *  2008-12-30 [JDB] Fixed bug wherein "pdf_write_jpeg_content_callback" called
 *                   "pdf_stream_printf" to write XObject name without escaping
 *                   restricted characters.  Now calls "pdf_write_name".
 */


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


struct pdf_jpeg_image
{
  double width, height;
  double x, y;
  bool color;  /* false for grayscale */
  uint32_t width_samples, height_samples;
  FILE *f;
  char XObject_name [4];
};


static void pdf_write_jpeg_content_callback (pdf_file_handle pdf_file,
					     struct pdf_obj *stream,
					     void *app_data)
{
  struct pdf_jpeg_image *image = app_data;

  /* transformation matrix is: width 0 0 height x y cm */
  pdf_stream_printf (pdf_file, stream, "q %g 0 0 %g %g %g cm ",
		     image->width, image->height,
		     image->x, image->y);
  pdf_write_name (pdf_file, image->XObject_name);
  pdf_stream_printf (pdf_file, stream, "Do Q\r\n");
}


#define JPEG_BUFFER_SIZE 8192

static void pdf_write_jpeg_image_callback (pdf_file_handle pdf_file,
					   struct pdf_obj *stream,
					   void *app_data)
{
  struct pdf_jpeg_image *image = app_data;
  int rlen, wlen;
  uint8_t *wp;
  uint8_t buffer [8192];

  while (! feof (image->f))
    {
      rlen = fread (& buffer [0], 1, JPEG_BUFFER_SIZE, image->f);
      wp = & buffer [0];
      while (rlen)
	{
	  wlen = fwrite (wp, 1, rlen, pdf_file->f);
	  if (feof (pdf_file->f))
	    pdf_fatal ("unexpected EOF on output file\n");
	  if (ferror (pdf_file->f))
	    pdf_fatal ("error on output file\n");
	  rlen -= wlen;
	  wp += wlen;
	}
      if (ferror (image->f))
	pdf_fatal ("error on input file\n");
    }
}


void pdf_write_jpeg_image (pdf_page_handle pdf_page,
			   double x,
			   double y,
			   double width,
			   double height,
			   bool color,
			   uint32_t width_samples,
			   uint32_t height_samples,
			   rgb_range_t *transparency,
			   FILE *f)
{
  struct pdf_jpeg_image *image;

  struct pdf_obj *stream;
  struct pdf_obj *stream_dict;

  struct pdf_obj *content_stream;

  struct pdf_obj *contents;
  struct pdf_obj *mask;

  image = pdf_calloc (1, sizeof (struct pdf_jpeg_image));

  image->width = width;
  image->height = height;
  image->x = x;
  image->y = y;

  image->f = f;

  image->color = color;
  image->width_samples = width_samples;
  image->height_samples = height_samples;

  pdf_add_array_elem_unique (pdf_page->procset,
			     pdf_new_name (image->color ? "ImageC" : "ImageB"));

  stream_dict = pdf_new_obj (PT_DICTIONARY);

  stream = pdf_new_ind_ref (pdf_page->pdf_file,
			    pdf_new_stream (pdf_page->pdf_file,
					    stream_dict,
					    & pdf_write_jpeg_image_callback,
					    image));

  strcpy (& image->XObject_name [0], "Im ");
  image->XObject_name [2] = pdf_new_XObject (pdf_page, stream);

  pdf_set_dict_entry (stream_dict, "Type",    pdf_new_name ("XObject"));
  pdf_set_dict_entry (stream_dict, "Subtype", pdf_new_name ("Image"));
  pdf_set_dict_entry (stream_dict, "Width",   pdf_new_integer (image->width_samples));
  pdf_set_dict_entry (stream_dict, "Height",  pdf_new_integer (image->height_samples));
  pdf_set_dict_entry (stream_dict, "ColorSpace", pdf_new_name (image->color ? "DeviceRGB" : "DeviceGray"));
  pdf_set_dict_entry (stream_dict, "BitsPerComponent", pdf_new_integer (8));

  if (transparency)
    {
      mask = pdf_new_obj (PT_ARRAY);
      
      pdf_add_array_elem (mask, pdf_new_integer (transparency->red.first));
      pdf_add_array_elem (mask, pdf_new_integer (transparency->red.last));

      if (image->color) {
	pdf_add_array_elem (mask, pdf_new_integer (transparency->green.first));
	pdf_add_array_elem (mask, pdf_new_integer (transparency->green.last));
	pdf_add_array_elem (mask, pdf_new_integer (transparency->blue.first));
	pdf_add_array_elem (mask, pdf_new_integer (transparency->blue.last));
      }

      pdf_set_dict_entry (stream_dict, "Mask", mask);
    }

  pdf_stream_add_filter (stream, "DCTDecode", NULL);

  /* the following will write the stream, using our callback function to
     get the actual data */
  pdf_write_ind_obj (pdf_page->pdf_file, stream);

  content_stream = pdf_new_ind_ref (pdf_page->pdf_file,
				    pdf_new_stream (pdf_page->pdf_file,
						    pdf_new_obj (PT_DICTIONARY),
						    & pdf_write_jpeg_content_callback,
						    image));

  contents = pdf_get_dict_entry (pdf_page->page_dict, "Contents");

  if (! contents)
    contents = pdf_new_obj (PT_ARRAY);

  pdf_add_array_elem (contents, content_stream);
  pdf_set_dict_entry (pdf_page->page_dict, "Contents", contents);

  pdf_write_ind_obj (pdf_page->pdf_file, content_stream);
}
