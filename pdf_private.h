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
 */


struct pdf_page
{
  pdf_file_handle pdf_file;
  pdf_obj_handle page_dict;
  pdf_obj_handle media_box;
  pdf_obj_handle procset;
  pdf_obj_handle resources;

  char last_XObject_name;
  pdf_obj_handle XObject_dict;
};


struct pdf_pages
{
  pdf_obj_handle pages_dict;
  pdf_obj_handle kids;
  pdf_obj_handle count;
};


struct pdf_file
{
  FILE                 *f;
  pdf_obj_handle       first_ind_obj;
  pdf_obj_handle       last_ind_obj;
  long int             xref_offset;
  pdf_obj_handle       catalog;
  pdf_obj_handle       info;
  struct pdf_pages     *root;
  struct pdf_bookmark  *outline_root;
  pdf_obj_handle       trailer_dict;
  struct pdf_name_tree *page_label_tree;
  struct pdf_name_tree *name_tree_list;
};
