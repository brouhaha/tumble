/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * PDF routines
 * $Id: pdf.c,v 1.4 2003/02/21 02:49:11 eric Exp $
 * Copyright 2001, 2002, 2003 Eric Smith <eric@brouhaha.com>
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


#include "bitblt.h"
#include "pdf.h"
#include "pdf_util.h"
#include "pdf_prim.h"
#include "pdf_private.h"


static void pdf_set_info (pdf_file_handle pdf_file, char *key, char *val)
{
  if (! pdf_file->info)
    pdf_file->info = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_DICTIONARY));

  pdf_set_dict_entry (pdf_file->info, key, pdf_new_string (val));
}


void pdf_init (void)
{
}


struct pdf_pages *pdf_new_pages (pdf_file_handle pdf_file)
{
  struct pdf_pages *pages = pdf_calloc (1, sizeof (struct pdf_pages));
  pages->kids = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_ARRAY));
  pages->count = pdf_new_integer (0);
  pages->pages_dict = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_DICTIONARY));
  pdf_set_dict_entry (pages->pages_dict, "Type", pdf_new_name ("Pages"));
  pdf_set_dict_entry (pages->pages_dict, "Kids", pages->kids);
  pdf_set_dict_entry (pages->pages_dict, "Count", pages->count);
  return (pages);
}


pdf_file_handle pdf_create (char *filename)
{
  pdf_file_handle pdf_file;

  pdf_file = pdf_calloc (1, sizeof (struct pdf_file));

  pdf_file->f = fopen (filename, "wb");
  if (! pdf_file->f)
    {
      pdf_fatal ("error opening output file\n");
    }

  pdf_file->root = pdf_new_pages (pdf_file);

  pdf_file->catalog = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_DICTIONARY));
  pdf_set_dict_entry (pdf_file->catalog, "Type", pdf_new_name ("Catalog"));
  pdf_set_dict_entry (pdf_file->catalog, "Pages", pdf_file->root->pages_dict);
  /* Outlines dictionary will be created later if needed*/
  pdf_set_dict_entry (pdf_file->catalog, "PageMode", pdf_new_name ("UseNone"));

  pdf_file->info    = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_DICTIONARY));
  pdf_set_info (pdf_file, "Producer", "t2p, Copyright 2003 Eric Smith <eric@brouhaha.com>");

  pdf_file->trailer_dict = pdf_new_obj (PT_DICTIONARY);
  /* Size key will be added later */
  pdf_set_dict_entry (pdf_file->trailer_dict, "Root", pdf_file->catalog);
  pdf_set_dict_entry (pdf_file->trailer_dict, "Info", pdf_file->info);

  /* write file header */
  fprintf (pdf_file->f, "%%PDF-1.2\r\n");

  return (pdf_file);
}


void pdf_close (pdf_file_handle pdf_file)
{
  /* write body */
  pdf_write_all_ind_obj (pdf_file);

  /* write cross reference table and get maximum object number */
  pdf_set_dict_entry (pdf_file->trailer_dict, "Size", pdf_new_integer (pdf_write_xref (pdf_file)));

  /* write trailer */
  fprintf (pdf_file->f, "trailer\r\n");
  pdf_write_obj (pdf_file, pdf_file->trailer_dict);
  fprintf (pdf_file->f, "startxref\r\n");
  fprintf (pdf_file->f, "%ld\r\n", pdf_file->xref_offset);
  fprintf (pdf_file->f, "%%%%EOF\r\n");

  fclose (pdf_file->f);
  /* should free stuff here */
}


void pdf_set_author   (pdf_file_handle pdf_file, char *author)
{
  pdf_set_info (pdf_file, "Author", author);
}

void pdf_set_creator  (pdf_file_handle pdf_file, char *creator)
{
  pdf_set_info (pdf_file, "Creator", creator);
}

void pdf_set_producer  (pdf_file_handle pdf_file, char *producer)
{
  pdf_set_info (pdf_file, "Producer", producer);
}

void pdf_set_title    (pdf_file_handle pdf_file, char *title)
{
  pdf_set_info (pdf_file, "Title", title);
}

void pdf_set_subject  (pdf_file_handle pdf_file, char *subject)
{
  pdf_set_info (pdf_file, "Subject", subject);
}

void pdf_set_keywords (pdf_file_handle pdf_file, char *keywords)
{
  pdf_set_info (pdf_file, "Keywords", keywords);
}


pdf_page_handle pdf_new_page (pdf_file_handle pdf_file,
			      double width,
			      double height)
{
  pdf_page_handle page = pdf_calloc (1, sizeof (struct pdf_page));

  page->pdf_file = pdf_file;

  page->media_box = pdf_new_obj (PT_ARRAY);
  pdf_add_array_elem (page->media_box, pdf_new_real (0));
  pdf_add_array_elem (page->media_box, pdf_new_real (0));
  pdf_add_array_elem (page->media_box, pdf_new_real (width));
  pdf_add_array_elem (page->media_box, pdf_new_real (height));

  page->procset = pdf_new_obj (PT_ARRAY);
  pdf_add_array_elem (page->procset, pdf_new_name ("PDF"));

  page->resources = pdf_new_obj (PT_DICTIONARY);
  pdf_set_dict_entry (page->resources, "ProcSet", page->procset);

  page->page_dict = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_DICTIONARY));
  pdf_set_dict_entry (page->page_dict, "Type", pdf_new_name ("Page"));
  pdf_set_dict_entry (page->page_dict, "MediaBox", page->media_box);
  pdf_set_dict_entry (page->page_dict, "Resources", page->resources);

  /* $$$ currently only support a single-level pages tree */
  pdf_set_dict_entry (page->page_dict, "Parent", pdf_file->root->pages_dict);
  pdf_add_array_elem (pdf_file->root->kids, page->page_dict);
  pdf_set_integer (pdf_file->root->count,
		   pdf_get_integer (pdf_file->root->count) + 1);

  page->last_XObject_name = '@';  /* first name will be "ImA" */

  return (page);
}

void pdf_close_page (pdf_page_handle pdf_page)
{
}


void pdf_set_page_number (pdf_page_handle pdf_page, char *page_number)
{
}

void pdf_bookmark (pdf_page_handle pdf_page, int level, char *name)
{
}
