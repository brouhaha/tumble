/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * PDF routines
 * $Id: bitblt_g4.c,v 1.7 2003/03/04 17:58:31 eric Exp $
 * Copyright 2001, 2002, 2003 Eric Smith <eric@brouhaha.com>
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


#include "pdf_g4_tables.h"


#define SWAP(type,a,b) do { type temp; temp = a; a = b; b = temp; } while (0)


struct pdf_g4_image
{
  double width, height;
  double x, y;
  double r, g, b;  /* fill color, only for ImageMask */
  unsigned long Columns;
  unsigned long Rows;
  bool ImageMask;
  bool BlackIs1;
  Bitmap *bitmap;
  char XObject_name [4];
};


char pdf_new_XObject (pdf_page_handle pdf_page, struct pdf_obj *ind_ref)
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


void pdf_write_g4_content_callback (pdf_file_handle pdf_file,
				    struct pdf_obj *stream,
				    void *app_data)
{
  struct pdf_g4_image *image = app_data;

  /* transformation matrix is: width 0 0 height x y cm */
  pdf_stream_printf (pdf_file, stream, "q %g 0 0 %g %g %g cm ",
		     image->width, image->height,
		     image->x, image->y);
  if (image->ImageMask)
    pdf_stream_printf (pdf_file, stream, "%g %g %g rg ",
		       image->r, image->g, image->b);

  pdf_stream_printf (pdf_file, stream, "/%s Do Q\r\n",
		     image->XObject_name);
}


static void pdf_g4_encode_horizontal_run (pdf_file_handle pdf_file,
					  struct pdf_obj *stream,
					  bool black,
					  uint32_t run_length)
{
  uint32_t i;

  while (run_length >= 2560)
    {
      pdf_stream_write_bits (pdf_file, stream, 12, 0x01f);
      run_length -= 2560;
    }

  if (run_length >= 1792)
    {
      i = (run_length - 1792) >> 6;
      pdf_stream_write_bits (pdf_file, stream,
			     g4_long_makeup_code [i].count,
			     g4_long_makeup_code [i].bits);
      run_length -= (1792 + (i << 6));
    }
  else if (run_length >= 64)
    {
      i = (run_length >> 6) - 1;
      pdf_stream_write_bits (pdf_file, stream,
			     g4_makeup_code [black] [i].count,
			     g4_makeup_code [black] [i].bits);
      run_length -= (i + 1) << 6;
    }

  pdf_stream_write_bits (pdf_file, stream,
			 g4_h_code [black] [run_length].count,
			 g4_h_code [black] [run_length].bits);
}


uint32_t find_transition (uint8_t *data,
			  uint32_t pos,
			  uint32_t width)
{
  if (! data)
    return (width);
  return (0);  /* $$$ */
}


static void pdf_g4_encode_row (pdf_file_handle pdf_file,
			       struct pdf_obj *stream,
			       uint32_t width,
			       uint8_t *ref,
			       uint8_t *row)
{
  int a0, a1, a2;
  int b1, b2;

  a0 = -1;
  
  while (a0 < width)
    {
      /* find a1, a2 */
      a1 = find_transition (row, a0, width);
      a2 = find_transition (row, a1, width);

      /* find b1, b2 */
      b1 = find_transition (ref, a0, width);
      if (0) /* $$$ b1 color = a0 color */
	b1 = find_transition (ref, b1, width);
      b2 = find_transition (ref, b2, width);

      if (b2 < a1)
	{
	  /* pass mode - 0001 */
	  pdf_stream_write_bits (pdf_file, stream, 4, 0x1);
	  a0 = b2;
	}
      else if (abs (a1 - b1) <= 3)
	{
	  /* vertical mode */
	  pdf_stream_write_bits (pdf_file, stream,
				 g4_vert_code [3 + a1 - b1].count,
				 g4_vert_code [3 + a1 - b1].bits);
	  a0 = a1;
	}
      else
	{
	  /* horizontal mode - 001 */
	  pdf_stream_write_bits (pdf_file, stream, 3, 0x1);
	  pdf_g4_encode_horizontal_run (pdf_file, stream,
					0 /* $$$ color (a0) */, a1 - a0);
	  pdf_g4_encode_horizontal_run (pdf_file, stream,
					1 /* $$$ color (a1) */, a2 - a1);
	  a0 = a2;
	}
    }
}


void pdf_write_g4_fax_image_callback (pdf_file_handle pdf_file,
				      struct pdf_obj *stream,
				      void *app_data)
{
  struct pdf_g4_image *image = app_data;

  uint32_t row;

  word_type *ref_line = NULL;  /* reference (previous) row */
  word_type *line = image->bitmap->bits;

  for (row = image->bitmap->rect.min.y;
       row < image->bitmap->rect.max.y;
       row++)
    {
      pdf_g4_encode_row (pdf_file, stream, image->Columns,
			 (uint8_t *) ref_line,
			 (uint8_t *) line);
      ref_line = line;
      line += image->bitmap->row_words;
    }

  
  /* write EOFB code */
  pdf_stream_write_bits (pdf_file, stream, 24, 0x001001);

  pdf_stream_flush_bits (pdf_file, stream);
}


void pdf_write_g4_fax_image (pdf_page_handle pdf_page,
			     double x,
			     double y,
			     double width,
			     double height,
			     Bitmap *bitmap,
			     bool ImageMask,
			     double r, /* RGB fill color, only for ImageMask */
			     double g,
			     double b,
			     bool BlackIs1)          /* boolean, typ. false */
{
  struct pdf_g4_image *image;

  struct pdf_obj *stream;
  struct pdf_obj *stream_dict;
  struct pdf_obj *decode_parms;

  struct pdf_obj *content_stream;

  image = pdf_calloc (1, sizeof (struct pdf_g4_image));

  image->width = width;
  image->height = height;
  image->x = x;
  image->y = y;
  image->r = r;
  image->g = g;
  image->b = b;

  image->bitmap = bitmap;
  image->Columns = bitmap->rect.max.x - bitmap->rect.min.x;
  image->Rows = bitmap->rect.max.y - bitmap->rect.min.y;
  image->ImageMask = ImageMask;
  image->BlackIs1 = BlackIs1;

  stream_dict = pdf_new_obj (PT_DICTIONARY);

  stream = pdf_new_ind_ref (pdf_page->pdf_file,
			    pdf_new_stream (pdf_page->pdf_file,
					    stream_dict,
					    & pdf_write_g4_fax_image_callback,
					    image));

  strcpy (& image->XObject_name [0], "Im ");
  image->XObject_name [2] = pdf_new_XObject (pdf_page, stream);

  pdf_set_dict_entry (stream_dict, "Type",    pdf_new_name ("XObject"));
  pdf_set_dict_entry (stream_dict, "Subtype", pdf_new_name ("Image"));
  pdf_set_dict_entry (stream_dict, "Name",    pdf_new_name (& image->XObject_name [0]));
  pdf_set_dict_entry (stream_dict, "Width",   pdf_new_integer (image->Columns));
  pdf_set_dict_entry (stream_dict, "Height",  pdf_new_integer (image->Rows));
  pdf_set_dict_entry (stream_dict, "BitsPerComponent", pdf_new_integer (1));
  if (ImageMask)
    pdf_set_dict_entry (stream_dict, "ImageMask", pdf_new_bool (ImageMask));
  else
    pdf_set_dict_entry (stream_dict, "ColorSpace", pdf_new_name ("DeviceGray"));

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

  if (BlackIs1)
    pdf_set_dict_entry (decode_parms,
			"BlackIs1",
			pdf_new_bool (BlackIs1));

  pdf_stream_add_filter (stream, "CCITTFaxDecode", decode_parms);

  /* the following will write the stream, using our callback function to
     get the actual data */
  pdf_write_ind_obj (pdf_page->pdf_file, stream);

  content_stream = pdf_new_ind_ref (pdf_page->pdf_file,
				    pdf_new_stream (pdf_page->pdf_file,
						    pdf_new_obj (PT_DICTIONARY),
						    & pdf_write_g4_content_callback,
						    image));

  pdf_set_dict_entry (pdf_page->page_dict, "Contents", content_stream);

  pdf_write_ind_obj (pdf_page->pdf_file, content_stream);
}

