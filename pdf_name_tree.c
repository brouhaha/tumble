/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * PDF routines
 * $Id: pdf_name_tree.c,v 1.3 2003/03/07 03:35:36 eric Exp $
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
#include "pdf_name_tree.h"


#define MAX_NAME_TREE_NODE_ENTRIES 16


struct pdf_name_tree_node
{
  struct pdf_obj *dict;    /* indirect reference */

  struct pdf_name_tree_node *parent;  /* NULL for root */
  bool leaf;

  int count;               /* how many kids or names/numbers are
			      attached to this node */

  struct pdf_name_tree_node *kids [MAX_NAME_TREE_NODE_ENTRIES];  /* non-leaf only */

  struct pdf_obj *min_key;
  struct pdf_obj *max_key;

  /* following fields valid in leaf nodes only: */

  struct pdf_obj *keys [MAX_NAME_TREE_NODE_ENTRIES];
  struct pdf_obj *values [MAX_NAME_TREE_NODE_ENTRIES];
};


struct pdf_name_tree *pdf_new_name_tree (pdf_file_handle pdf_file,
					 bool number_tree)
{
  struct pdf_name_tree *tree;
  struct pdf_name_tree_node *root;

  root = pdf_calloc (1, sizeof (struct pdf_name_tree_node));
  tree = pdf_calloc (1, sizeof (struct pdf_name_tree));

  tree->pdf_file = pdf_file;
  tree->root = root;
  tree->number_tree = number_tree;

  root->parent = NULL;
  root->leaf = 1;

  return (tree);
}


static void pdf_split_name_tree_node (struct pdf_name_tree *tree,
				      struct pdf_name_tree_node *node)
{
  struct pdf_name_tree_node *new_node;

  if (node == tree->root)
    {
      /* create new root above current root */
      struct pdf_name_tree_node *new_root_node;

      new_root_node = pdf_calloc (1, sizeof (struct pdf_name_tree_node));

      new_root_node->parent = NULL;
      new_root_node->leaf = 0;

      new_root_node->count = 1;
      new_root_node->kids [0] = node;

      new_root_node->min_key = node->min_key;
      new_root_node->max_key = node->max_key;

      node->parent = new_root_node;
      tree->root = new_root_node;
    }

  new_node = pdf_calloc (1, sizeof (struct pdf_name_tree_node));
  new_node->parent = node->parent;
  new_node->leaf = node->leaf;

  /* $$$ insert new node in parent's kids array */
}


static void pdf_add_tree_element (struct pdf_name_tree *tree,
				  struct pdf_obj *key,
				  struct pdf_obj *val)
{
  struct pdf_name_tree_node *node;
  int i;

  /* find node which should contain element */
  node = tree->root;
  while (! node->leaf)
    {
      for (i = 0; i < (node->count - 1); i++)
	if (pdf_compare_obj (key, node->kids [i + 1]->min_key) < 0)
	  break;
      node = node->kids [i];
    }

  /* if node is full, split, recursing to root if necessary */
  if (node->count == MAX_NAME_TREE_NODE_ENTRIES)
    {
      pdf_split_name_tree_node (tree, node);
      pdf_add_tree_element (tree, key, val);
      return;
    }

  /* figure out in which slot to insert it */
  for (i = 0; i < node->count; i++)
    if (pdf_compare_obj (key, node->keys [i] < 0))
      break;

  /* move other entries right one position */
  if (i != node->count)
    {
      memmove (& node->keys [i+1],
	       & node->keys [i],
	       (node->count - i) * sizeof (struct pdf_obj *));
      memmove (& node->values [i+1],
	       & node->values [i],
	       (node->count - i) * sizeof (struct pdf_obj *));
    }

  node->keys [i] = key;
  node->values [i] = val;

  node->count++;

  /* update limits, recursing upwards if necessary */
  if (i == 0)
    {
      node->min_key = key;
      while (node->parent && (node->parent->kids [0] == node))
	{
	  node = node->parent;
	  node->min_key = key;
	}
    }
  else if (i == (node->count - 1))
    {
      node->max_key = key;
      while (node->parent && (node->parent->kids [node->parent->count - 1] == node))
	{
	  node = node->parent;
	  node->max_key = key;
	}
    }
}


void pdf_add_name_tree_element (struct pdf_name_tree *tree,
				char *key,
				struct pdf_obj *val)
{
  struct pdf_obj *key_obj = pdf_new_string (key);
  pdf_add_tree_element (tree, key_obj, val);
}


void pdf_add_number_tree_element (struct pdf_name_tree *tree,
				  long key,
				  struct pdf_obj *val)
{
  struct pdf_obj *key_obj = pdf_new_integer (key);
  pdf_add_tree_element (tree, key_obj, val);
}


static void pdf_finalize_name_tree_node (struct pdf_name_tree *tree,
					 struct pdf_name_tree_node *node)
{
  int i;

  node->dict = pdf_new_ind_ref (tree->pdf_file, pdf_new_obj (PT_DICTIONARY));

  if (node->leaf)
    {
      /* write Names or Nums array */
      struct pdf_obj *names = pdf_new_obj (PT_ARRAY);
      for (i = 0; i < node->count; i++)
	{
	  pdf_add_array_elem (names, node->keys [i]);
	  pdf_add_array_elem (names, node->values [i]);
	}
      pdf_set_dict_entry (node->dict,
			  tree->number_tree ? "Nums" : "Names",
			  names);
    }
  else
    {
      /* finalize the children first so that their dict ind ref is
	 available */
      for (i = 0; i < node->count; i++)
	pdf_finalize_name_tree_node (tree, node->kids [i]);

      /* write Kids array */
      struct pdf_obj *kids = pdf_new_obj (PT_ARRAY);
      for (i = 0; i < node->count; i++)
	pdf_add_array_elem (kids, node->kids [i]->dict);
      pdf_set_dict_entry (node->dict, "Kids", kids);
    }

  if (! node->parent)
    {
      /* write Limits array */
      struct pdf_obj *limits = pdf_new_obj (PT_ARRAY);
      pdf_add_array_elem (limits, node->min_key);
      pdf_add_array_elem (limits, node->max_key);
      pdf_set_dict_entry (node->dict, "Limits", limits);
    }
}


void pdf_finalize_name_tree (struct pdf_name_tree *tree)
{
  pdf_finalize_name_tree_node (tree, tree->root);
}
