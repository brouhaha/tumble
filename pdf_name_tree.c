/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * PDF routines
 * $Id: pdf_name_tree.c,v 1.8 2003/03/12 03:13:28 eric Exp $
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

  tree->next = pdf_file->name_tree_list;
  pdf_file->name_tree_list = tree;

  return (tree);
}


static void pdf_split_name_tree_node (struct pdf_name_tree *tree,
				      struct pdf_name_tree_node *node)
{
  struct pdf_name_tree_node *parent;
  struct pdf_name_tree_node *new_node;
  int i, j;

  parent = node->parent;
  if (! parent)
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

      parent = new_root_node;
      node->parent = new_root_node;
      tree->root = new_root_node;
    }

  if (parent->count == MAX_NAME_TREE_NODE_ENTRIES)
    {
      pdf_split_name_tree_node (tree, parent);
      parent = node->parent;
    }

  new_node = pdf_calloc (1, sizeof (struct pdf_name_tree_node));
  new_node->parent = parent;
  new_node->leaf = node->leaf;

  /* move half the node's entries into the new node */
  i = node->count / 2;
  j = node->count - i;

  memcpy (& new_node->kids [0],
	  & node->kids [i],
	  j * sizeof (struct pdf_name_tree_node *));
  memcpy (& new_node->keys [0],
	  & node->keys [i],
	  j * sizeof (struct pdf_obj *));
  memcpy (& new_node->values [0],
	  & node->values [i],
	  j * sizeof (struct pdf_obj *));

  node->count = i;
  new_node->count = j;

  if (! new_node->leaf)
    for (i = 0; i < j; i++)
      new_node->kids [i]->parent = new_node;

  /* set max_key of the old node */
  if (node->leaf)
    node->max_key = node->keys [node->count - 1];
  else
    node->max_key = node->kids [node->count - 1]->max_key;

  /* set min_key and max_key in the new node */
  if (new_node->leaf)
    {
      new_node->min_key = new_node->keys [0];
      new_node->max_key = new_node->keys [new_node->count - 1];
    }
  else
    {
      new_node->min_key = new_node->kids [0]->min_key;
      new_node->max_key = new_node->kids [new_node->count - 1]->max_key;
    }

  /* insert new node in parent's kids array */
  /* find entry of old node */
  for (i = 0; i < parent->count; i++)
    if (parent->kids [i] == node)
      break;

  /* it had better have been there! */
  pdf_assert (i < parent->count);

  /* the new node goes in the slot to the right of the old node */
  i++;

  /* move other entries right one position */
  if (i != parent->count)
    {
      memmove (& parent->kids [i+1],
	       & parent->kids [i],
	       (parent->count - i) * sizeof (struct pdf_name_tree_node *));
    }

  parent->kids [i] = new_node;
  parent->count++;
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
    if (pdf_compare_obj (key, node->keys [i]) < 0)
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
  if (i == (node->count - 1))
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
      struct pdf_obj *kids;

      for (i = 0; i < node->count; i++)
	pdf_finalize_name_tree_node (tree, node->kids [i]);

      /* write Kids array */
      kids = pdf_new_obj (PT_ARRAY);
      for (i = 0; i < node->count; i++)
	pdf_add_array_elem (kids, node->kids [i]->dict);
      pdf_set_dict_entry (node->dict, "Kids", kids);
    }

  if (node->parent)
    {
      /* write Limits array */
      struct pdf_obj *limits = pdf_new_obj (PT_ARRAY);
      pdf_add_array_elem (limits, node->min_key);
      pdf_add_array_elem (limits, node->max_key);
      pdf_set_dict_entry (node->dict, "Limits", limits);
    }
}


void pdf_finalize_name_trees (pdf_file_handle pdf_file)
{
  struct pdf_name_tree *tree;

  for (tree = pdf_file->name_tree_list; tree; tree = tree->next)
    pdf_finalize_name_tree_node (tree, tree->root);
}
