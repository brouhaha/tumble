/*
 * tumble: build a PDF file from image files
 *
 * PDF routines
 * $Id: pdf_jpeg.c,v 1.2 2003/03/13 00:57:05 eric Exp $
 * Copyright 2003 Eric Smith <eric@brouhaha.com>
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
  FILE *f;
  unsigned long Columns;
  unsigned long Rows;
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
  pdf_stream_printf (pdf_file, stream, "/%s Do Q\r\n",
		     image->XObject_name);
}


#define JPEG_BUFFER_SIZE 8192

static void pdf_write_jpeg_image_callback (pdf_file_handle pdf_file,
					   struct pdf_obj *stream,
					   void *app_data)
{
  struct pdf_jpeg_image *image = app_data;
  FILE *f;
  int rlen, wlen;
  uint8_t *wp;
  uint8_t buffer [8192];

  while (! feof (image->f))
    {
      rlen = fread (& buffer [0], 1, JPEG_BUFFER_SIZE, f);
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
      if (ferror (f))
	pdf_fatal ("error on input file\n");
    }
}


void pdf_write_jpeg_image (pdf_page_handle pdf_page,
			   double x,
			   double y,
			   double width,
			   double height,
			   FILE *f)
{
  struct pdf_jpeg_image *image;

  struct pdf_obj *stream;
  struct pdf_obj *stream_dict;
  struct pdf_obj *decode_parms;

  struct pdf_obj *content_stream;

  image = pdf_calloc (1, sizeof (struct pdf_jpeg_image));

  image->width = width;
  image->height = height;
  image->x = x;
  image->y = y;

  image->f = f;
#if 0
  image->Columns = bitmap->rect.max.x - bitmap->rect.min.x;
  image->Rows = bitmap->rect.max.y - bitmap->rect.min.y;
#endif

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
  pdf_set_dict_entry (stream_dict, "Name",    pdf_new_name (& image->XObject_name [0]));
  pdf_set_dict_entry (stream_dict, "Width",   pdf_new_integer (image->Columns));
  pdf_set_dict_entry (stream_dict, "Height",  pdf_new_integer (image->Rows));
  pdf_set_dict_entry (stream_dict, "BitsPerComponent", pdf_new_integer (8));

  decode_parms = pdf_new_obj (PT_DICTIONARY);

  pdf_set_dict_entry (decode_parms,
		      "K",
		      pdf_new_integer (-1));

  pdf_set_dict_entry (decode_parms,
		      "Columns",
		      pdf_new_integer (image->Columns));

  pdf_set_dict_entry (decode_parms,
		      "Rows",
		      pdf_new_integer (image->Rows));

  pdf_stream_add_filter (stream, "DCTDecode", decode_parms);

  /* the following will write the stream, using our callback function to
     get the actual data */
  pdf_write_ind_obj (pdf_page->pdf_file, stream);

  content_stream = pdf_new_ind_ref (pdf_page->pdf_file,
				    pdf_new_stream (pdf_page->pdf_file,
						    pdf_new_obj (PT_DICTIONARY),
						    & pdf_write_jpeg_content_callback,
						    image));

  pdf_set_dict_entry (pdf_page->page_dict, "Contents", content_stream);

  pdf_write_ind_obj (pdf_page->pdf_file, content_stream);
}

