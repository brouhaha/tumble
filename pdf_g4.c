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
 *  2008-12-30 [JDB] Fixed bug wherein "pdf_write_g4_content_callback" called
 *                   "pdf_stream_printf" to write XObject name without escaping
 *                   restricted characters.  Now calls "pdf_write_name".
 *
 *  2010-09-02 [JDB] Added support for min-is-black TIFF images.
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


struct pdf_g4_image
{
  double width, height;
  double x, y;
  unsigned long Columns;
  unsigned long Rows;
  Bitmap *bitmap;
  char XObject_name [4];
};


static void pdf_write_g4_content_callback (pdf_file_handle pdf_file,
					   struct pdf_obj *stream,
					   void *app_data)
{
  struct pdf_g4_image *image = app_data;

  /* transformation matrix is: width 0 0 height x y cm */
  pdf_stream_printf (pdf_file, stream, "q %g 0 0 %g %g %g cm ",
		     image->width, image->height,
		     image->x, image->y);

  pdf_write_name (pdf_file, image->XObject_name);
  pdf_stream_printf (pdf_file, stream, "Do Q\r\n");
}


static void pdf_write_g4_fax_image_callback (pdf_file_handle pdf_file,
					     struct pdf_obj *stream,
					     void *app_data)
{
  struct pdf_g4_image *image = app_data;

  bitblt_write_g4 (image->bitmap, pdf_file->f);
}


void pdf_write_g4_fax_image (pdf_page_handle pdf_page,
			     double x,
			     double y,
			     double width,
			     double height,
			     bool negative,
			     Bitmap *bitmap,
			     colormap_t *colormap,
			     rgb_range_t *transparency)
{
  struct pdf_g4_image *image;

  struct pdf_obj *stream;
  struct pdf_obj *stream_dict;
  struct pdf_obj *decode_parms;

  struct pdf_obj *content_stream;

  struct pdf_obj *contents;
  struct pdf_obj *mask;
  
  typedef char MAP_STRING[6];
  
  MAP_STRING color_index;
  static MAP_STRING last_color_index;
  static struct pdf_obj *color_space;


  pdf_add_array_elem_unique (pdf_page->procset, pdf_new_name ("ImageB"));

  image = pdf_calloc (1, sizeof (struct pdf_g4_image));

  image->width = width;
  image->height = height;
  image->x = x;
  image->y = y;

  image->bitmap = bitmap;
  image->Columns = bitmap->rect.max.x - bitmap->rect.min.x;
  image->Rows = bitmap->rect.max.y - bitmap->rect.min.y;

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

  if (transparency)
    {
      mask = pdf_new_obj (PT_ARRAY);
      
      pdf_add_array_elem (mask, pdf_new_integer (transparency->red.first));
      pdf_add_array_elem (mask, pdf_new_integer (transparency->red.last));

      pdf_set_dict_entry (stream_dict, "Mask", mask);
    }

  if (colormap)
    {
      color_index [0] = (char) colormap->black_map.red;
      color_index [1] = (char) colormap->black_map.green;
      color_index [2] = (char) colormap->black_map.blue;
      color_index [3] = (char) colormap->white_map.red;
      color_index [4] = (char) colormap->white_map.green;
      color_index [5] = (char) colormap->white_map.blue;

      if ((color_space == NULL) || 
          (memcmp (color_index, last_color_index, sizeof (MAP_STRING)) != 0))
	{
	  memcpy (last_color_index, color_index, sizeof (MAP_STRING));

	  color_space = pdf_new_obj (PT_ARRAY);
	  pdf_add_array_elem (color_space, pdf_new_name ("Indexed"));
	  pdf_add_array_elem (color_space, pdf_new_name ("DeviceRGB"));
	  pdf_add_array_elem (color_space, pdf_new_integer (1));
	  pdf_add_array_elem (color_space, pdf_new_string_n (color_index, 6));

	  color_space = pdf_new_ind_ref (pdf_page->pdf_file, color_space);
	}

      pdf_set_dict_entry (stream_dict, "ColorSpace", color_space);
    }
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

  if (negative)
    pdf_set_dict_entry (decode_parms,
			"BlackIs1",
			pdf_new_bool (true));

  pdf_stream_add_filter (stream, "CCITTFaxDecode", decode_parms);

  /* the following will write the stream, using our callback function to
     get the actual data */
  pdf_write_ind_obj (pdf_page->pdf_file, stream);

  content_stream = pdf_new_ind_ref (pdf_page->pdf_file,
				    pdf_new_stream (pdf_page->pdf_file,
						    pdf_new_obj (PT_DICTIONARY),
						    & pdf_write_g4_content_callback,
						    image));

  contents = pdf_get_dict_entry (pdf_page->page_dict, "Contents");

  if (! contents)
    contents = pdf_new_obj (PT_ARRAY);

  pdf_add_array_elem (contents, content_stream);
  pdf_set_dict_entry (pdf_page->page_dict, "Contents", contents);

  pdf_write_ind_obj (pdf_page->pdf_file, content_stream);
}

