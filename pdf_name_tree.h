/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * PDF routines
 * $Id: pdf_name_tree.h,v 1.1 2003/03/07 02:16:08 eric Exp $
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


struct pdf_name_tree
{
  pdf_file_handle   pdf_file;
  bool              number_tree;   /* false for name tree,
				      true for number tree */
  struct pdf_name_tree_node *root;
};


struct pdf_name_tree *pdf_new_name_tree (pdf_file_handle pdf_file,
					 bool number_tree);


void pdf_add_name_tree_element (struct pdf_name_tree *tree,
				char *key,
				struct pdf_obj *val);


void pdf_add_number_tree_element (struct pdf_name_tree *tree,
				  long key,
				  struct pdf_obj *val);


void pdf_finalize_name_tree (struct pdf_name_tree *tree);
