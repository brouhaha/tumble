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


struct pdf_name_tree
{
  pdf_file_handle           pdf_file;
  struct pdf_name_tree      *next;  /* chain all name trees in the PDF file */
  bool                      number_tree;   /* false for name tree,
					      true for number tree */
  struct pdf_name_tree_node *root;
};


#define MAX_NAME_TREE_NODE_ENTRIES 32


struct pdf_name_tree_node
{
  pdf_obj_handle dict;    /* indirect reference */

  struct pdf_name_tree_node *parent;  /* NULL for root */
  bool leaf;

  int count;               /* how many kids or names/numbers are
			      attached to this node */

  struct pdf_name_tree_node *kids [MAX_NAME_TREE_NODE_ENTRIES];  /* non-leaf only */

  pdf_obj_handle min_key;
  pdf_obj_handle max_key;

  /* following fields valid in leaf nodes only: */

  pdf_obj_handle keys [MAX_NAME_TREE_NODE_ENTRIES];
  pdf_obj_handle values [MAX_NAME_TREE_NODE_ENTRIES];
};


struct pdf_name_tree *pdf_new_name_tree (pdf_file_handle pdf_file,
					 bool number_tree);


void pdf_add_name_tree_element (struct pdf_name_tree *tree,
				char *key,
				pdf_obj_handle val);


void pdf_add_number_tree_element (struct pdf_name_tree *tree,
				  long key,
				  pdf_obj_handle val);


void pdf_finalize_name_trees (pdf_file_handle pdf_file);
