/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * PDF routines
 * $Id: pdf_bookmark.c,v 1.5 2003/03/05 12:56:25 eric Exp $
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


struct pdf_bookmark
{
  struct pdf_obj *dict;    /* indirect reference */
  struct pdf_obj *count;
  bool open;

  struct pdf_bookmark *first;
  struct pdf_bookmark *last;

  /* the following fields don't appear in the root */
  /* title and dest are in the dictionary but don't have
     explicit fields in the C structure */
  struct pdf_bookmark *parent;
  struct pdf_bookmark *prev;
  struct pdf_bookmark *next;
};


static void pdf_bookmark_update_count (pdf_bookmark_handle entry)
{
  while (entry)
    {
      if (! entry->count)
	{
	  entry->count = pdf_new_integer (0);
	  pdf_set_dict_entry (entry->dict, "Count", entry->count);
	}
      pdf_set_integer (entry->count,
		       pdf_get_integer (entry->count) +
		       ((entry->open) ? 1 : -1));
      if (! entry->open)
	break;
      entry = entry->parent;
    }
}


/* Create a new bookmark, under the specified parent, or at the top
   level if parent is NULL. */
pdf_bookmark_handle pdf_new_bookmark (pdf_bookmark_handle parent,
				      char *title,
				      bool open,
				      pdf_page_handle pdf_page)
{
  pdf_file_handle pdf_file = pdf_page->pdf_file;
  struct pdf_bookmark *root;
  struct pdf_bookmark *entry;

  struct pdf_obj *dest_array;

  root = pdf_file->outline_root;
  if (! root)
    {
      root = pdf_calloc (1, sizeof (struct pdf_bookmark));
      root->dict = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_DICTIONARY));

      pdf_file->outline_root = root;
      pdf_set_dict_entry (pdf_file->catalog, "Outlines", root->dict);
    }

  entry = pdf_calloc (1, sizeof (struct pdf_bookmark));
  entry->dict = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_DICTIONARY));
  entry->open = open;

  pdf_set_dict_entry (entry->dict, "Title", pdf_new_string (title));

  dest_array = pdf_new_obj (PT_ARRAY);
  pdf_add_array_elem (dest_array, pdf_page->page_dict);
  pdf_add_array_elem (dest_array, pdf_new_name ("Fit"));
  pdf_set_dict_entry (entry->dict, "Dest", dest_array);

  if (parent)
    {
      entry->parent = parent;
      entry->prev = parent->last;
    }
  else
    {
      parent = root;
      entry->parent = root;
      entry->prev = root->last;
    }

  pdf_set_dict_entry (entry->dict, "Parent", parent->dict);

  if (entry->prev)
    {
      pdf_set_dict_entry (entry->dict, "Prev", entry->prev->dict);

      entry->prev->next = entry;
      pdf_set_dict_entry (entry->prev->dict, "Next", entry->dict);

      parent->last = entry;
      pdf_set_dict_entry (parent->dict, "Last", entry->dict);
    }
  else
    {
      parent->first = entry;
      pdf_set_dict_entry (parent->dict, "First", entry->dict);

      parent->last = entry;
      pdf_set_dict_entry (parent->dict, "Last", entry->dict);
    }

  pdf_bookmark_update_count (parent);

  return (entry);
}
