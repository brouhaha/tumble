/*
 * tumble: build a PDF file from image files
 *
 * PDF routines
 * $Id: pdf.c,v 1.13 2003/03/14 00:57:40 eric Exp $
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
 *
 *  2005-05-03 [JDB] Add CreationDate and ModDate PDF headers.
 *  2014-02-18 [JDB] Use PDF_PRODUCER definition.
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include "bitblt.h"
#include "pdf.h"
#include "pdf_util.h"
#include "pdf_prim.h"
#include "pdf_private.h"
#include "pdf_name_tree.h"


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

  pages->kids = pdf_new_obj (PT_ARRAY);
  /* The PDF 1.0 spec doesn't say that kids can't be an indirect object,
     but Acrobat 4.0 fails to optimize files if it is. */

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
  time_t current_time, adjusted_time;
  struct tm *time_and_date;
  int gmt_diff;
  char tm_string[18], gmt_string[24];
  const char tm_format[] = "D:%Y%m%d%H%M%S";

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
  /* Outlines dictionary will be created later if needed */
  pdf_set_dict_entry (pdf_file->catalog, "PageLayout", pdf_new_name ("SinglePage"));

  pdf_file->info    = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_DICTIONARY));
  pdf_set_info (pdf_file, "Producer", PDF_PRODUCER);

  /* Generate CreationDate and ModDate */

  current_time = time (NULL);
  time_and_date = gmtime (&current_time);
  adjusted_time = mktime (time_and_date);
  gmt_diff = (int) difftime (current_time, adjusted_time);

  time_and_date = localtime (&current_time);

  if (strftime (tm_string, sizeof (tm_string), tm_format, time_and_date))
    {
      sprintf (gmt_string, "%s%+03d'%02d'", tm_string, gmt_diff / 3600, gmt_diff % 60);
      pdf_set_info (pdf_file, "CreationDate", gmt_string);
      pdf_set_info (pdf_file, "ModDate", gmt_string);
    }

  pdf_file->trailer_dict = pdf_new_obj (PT_DICTIONARY);
  /* Size key will be added later */
  pdf_set_dict_entry (pdf_file->trailer_dict, "Root", pdf_file->catalog);
  pdf_set_dict_entry (pdf_file->trailer_dict, "Info", pdf_file->info);

  /* write file header */
  fprintf (pdf_file->f, "%%PDF-1.3\r\n");

  /* write comment containing 8-bit chars as a hint that the file is binary */
  /* PDF 1.4 spec, section 3.4.1 */
  fprintf (pdf_file->f, "%%\342\343\317\323\r\n");

  return (pdf_file);
}


void pdf_close (pdf_file_handle pdf_file, int page_mode)
{
  char *page_mode_string;

  page_mode_string = "UseNone";

  switch (page_mode)
    {
    case PDF_PAGE_MODE_USE_NONE:
      break;
    case PDF_PAGE_MODE_USE_OUTLINES:
      if (pdf_file->outline_root)
	page_mode_string = "UseOutlines";
      break;
    case PDF_PAGE_MODE_USE_THUMBS:
      page_mode_string = "UseThumbs";
      break;
    default:
      pdf_fatal ("invalid page mode\n");
    }

  pdf_set_dict_entry (pdf_file->catalog,
		      "PageMode",
		      pdf_new_name (page_mode_string));

  /* finalize trees, object numbers aren't allocated until this step */
  pdf_finalize_name_trees (pdf_file);

  /* add the page label number tree, if it exists, to the catalog */
  if (pdf_file->page_label_tree)
    pdf_set_dict_entry (pdf_file->catalog,
			"PageLabels",
			pdf_file->page_label_tree->root->dict);

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
  pdf_add_array_elem_unique (page->procset, pdf_new_name ("PDF"));

  page->resources = pdf_new_obj (PT_DICTIONARY);
  pdf_set_dict_entry (page->resources, "ProcSet", page->procset);

  page->page_dict = pdf_new_ind_ref (pdf_file, pdf_new_obj (PT_DICTIONARY));
  pdf_set_dict_entry (page->page_dict, "Type", pdf_new_name ("Page"));
  pdf_set_dict_entry (page->page_dict, "MediaBox", page->media_box);
  pdf_set_dict_entry (page->page_dict, "Resources", page->resources);

  if (pdf_get_integer (pdf_file->root->count) == 0)
    {
      struct pdf_obj *dest_array = pdf_new_obj (PT_ARRAY);
      pdf_add_array_elem (dest_array, page->page_dict);
      pdf_add_array_elem (dest_array, pdf_new_name ("Fit"));
      pdf_set_dict_entry (pdf_file->catalog, "OpenAction", dest_array);
    }

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

