/*
 * tumble: build a PDF file from image files
 *
 * $Id: tumble_blank.c ... Exp $
 * Copyright ...
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
 *  2007-05-07 [JDB] New file to add support for blank pages.
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


#include "semantics.h"
#include "tumble.h"
#include "bitblt.h"
#include "pdf.h"
#include "pdf_util.h"
#include "pdf_prim.h"
#include "pdf_private.h"
#include "tumble_input.h"


struct pdf_blank_page
{
  double width;
  double height;
  double x;
  double y;
  double red;
  double green;
  double blue;
};


static bool match_blank_suffix (char *suffix)
{
  return false;
}

static bool close_blank_input_file (void)
{
  return true;
}


static bool open_blank_input_file (FILE *f, char *name)
{
  return true;
}


static bool last_blank_input_page (void)
{
  return true;
}


static bool get_blank_image_info (int image,
				  input_attributes_t input_attributes,
				  image_info_t *image_info)
{
  if (input_attributes.has_page_size)
    {
      image_info->width_points = input_attributes.page_size.width * POINTS_PER_INCH;
      image_info->height_points = input_attributes.page_size.height * POINTS_PER_INCH;
      return true;
    }
  else
    return false;
}


static void pdf_write_blank_content_callback (pdf_file_handle pdf_file,
					      pdf_obj_handle stream,
					      void *app_data)
{
  struct pdf_blank_page *page = app_data;

  pdf_stream_printf (pdf_file, stream,
		     "%g %g %g rg\r\n%g %g %g %g re\r\nf\r\n",
		     page->red, page->green, page->blue,
		     page->x, page->y,
		     page->width, page->height);
}


static bool process_blank_image (int image,  /* range 1 .. n */
				 input_attributes_t input_attributes,
				 image_info_t *image_info,
				 pdf_page_handle pdf_page,
				 output_attributes_t output_attributes)
{
  struct pdf_blank_page *page;

/* If colormap set, use "white" color and draw rectangle to cover page. */

  if (output_attributes.colormap)
    {
      page = pdf_calloc (1, sizeof (struct pdf_blank_page));

      page->width = image_info->width_points;
      page->height = image_info->height_points;
      page->x = 0;
      page->y = 0;
      page->red   = (double) output_attributes.colormap->white_map.red / 255.0;
      page->green = (double) output_attributes.colormap->white_map.green / 255.0;
      page->blue  = (double) output_attributes.colormap->white_map.blue / 255.0;

      pdf_obj_handle content_stream = pdf_new_ind_ref(pdf_page->pdf_file,
						      pdf_new_stream (pdf_page->pdf_file,
								      pdf_new_obj (PT_DICTIONARY),
								      & pdf_write_blank_content_callback,
								      page));
      pdf_page_add_content_stream(pdf_page, content_stream);
    }
  return true;
}


input_handler_t blank_handler =
  {
    match_blank_suffix,
    open_blank_input_file,
    close_blank_input_file,
    last_blank_input_page,
    get_blank_image_info,
    process_blank_image
  };
