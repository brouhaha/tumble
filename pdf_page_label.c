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
 *  2007-05-07 [JDB] Fixed a bug wherein a page label specifying prefix without
 *                   a style (e.g., LABEL <prefix>) produced bad PDF (no labels
 *                   were displayed).  Should have output "/P <prefix>" but
 *                   instead output "/S /P <prefix>".
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
#include "pdf_name_tree.h"


void pdf_new_page_label (pdf_file_handle pdf_file,
			 int page_index,
			 int base,
			 int count,
			 char style,
			 char *prefix)
{
  struct pdf_obj *label_dict;
  char style_str [2] = { style, '\0' };

  if (! pdf_file->page_label_tree)
    {
      pdf_file->page_label_tree = pdf_new_name_tree (pdf_file, 1);
    }

  label_dict = pdf_new_obj (PT_DICTIONARY);

  if (style)
    pdf_set_dict_entry (label_dict, "S", pdf_new_name (style_str));

  if (prefix)
    pdf_set_dict_entry (label_dict, "P", pdf_new_string (prefix));

  if (base > 1)
    pdf_set_dict_entry (label_dict, "St", pdf_new_integer (base));

  pdf_add_number_tree_element (pdf_file->page_label_tree,
			       page_index,
			       label_dict);
}

