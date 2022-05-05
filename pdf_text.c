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



static void pdf_write_text_content_callback (pdf_file_handle pdf_file,
					     pdf_obj_handle stream,
					     void *app_data)
{
  pdf_stream_printf (pdf_file, stream,
		     "BT /F1 24 Tf 100 100 Td (Hello World) Tj ET\r\n");
}


static pdf_obj_handle pdf_create_font (pdf_page_handle pdf_page)
{
  pdf_obj_handle font = pdf_new_ind_ref (pdf_page->pdf_file,
					  pdf_new_obj (PT_DICTIONARY));
  pdf_set_dict_entry (font, "Type", pdf_new_name ("Font"));
  pdf_set_dict_entry (font, "Subtype", pdf_new_name ("Type1"));
  pdf_set_dict_entry (font, "Name", pdf_new_name ("F1"));
  pdf_set_dict_entry (font, "BaseFont", pdf_new_name ("Helvetica"));
  pdf_set_dict_entry (font, "Encoding", pdf_new_name ("MacRomanEncoding"));

  return (font);
}

void pdf_write_text (pdf_page_handle pdf_page)
{
  pdf_obj_handle font;
  pdf_obj_handle font_dict;

  font = pdf_create_font (pdf_page);

  font_dict = pdf_new_ind_ref (pdf_page->pdf_file,
			       pdf_new_obj (PT_DICTIONARY));

  pdf_set_dict_entry (font_dict, "F1", font);

  pdf_set_dict_entry (pdf_page->resources, "Font", font_dict);

  pdf_add_array_elem_unique (pdf_page->procset, pdf_new_name ("Text"));

  pdf_obj_handle content_stream = pdf_new_ind_ref(pdf_page->pdf_file,
						  pdf_new_stream (pdf_page->pdf_file,
								  pdf_new_obj (PT_DICTIONARY),
								  & pdf_write_text_content_callback,
								  NULL));

  pdf_page_add_content_stream(pdf_page, content_stream);
}

