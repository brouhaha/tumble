/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * PDF routines
 * $Id: pdf.h,v 1.5 2003/03/04 17:58:36 eric Exp $
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


typedef struct pdf_file *pdf_file_handle;

typedef struct pdf_page *pdf_page_handle;

typedef struct pdf_bookmark *pdf_bookmark_handle;


void pdf_init (void);

pdf_file_handle pdf_create (char *filename);

void pdf_close (pdf_file_handle pdf_file);

void pdf_set_author   (pdf_file_handle pdf_file, char *author);
void pdf_set_creator  (pdf_file_handle pdf_file, char *author);
void pdf_set_title    (pdf_file_handle pdf_file, char *author);
void pdf_set_subject  (pdf_file_handle pdf_file, char *author);
void pdf_set_keywords (pdf_file_handle pdf_file, char *author);


/* width and height in units of 1/72 inch */
pdf_page_handle pdf_new_page (pdf_file_handle pdf_file,
			      double width,
			      double height);

void pdf_close_page (pdf_page_handle pdf_page);


/* The length of the data must be Rows * rowbytes.
   Note that rowbytes must be at least (Columns+7)/8, but may be arbitrarily
   large. */
void pdf_write_g4_fax_image (pdf_page_handle pdf_page,
			     double x,
			     double y,
			     double width,
			     double height,
			     Bitmap *bitmap,
			     bool ImageMask,
			     double r, /* RGB fill color, only for ImageMask */
			     double g,
			     double b,
			     bool BlackIs1);    /* boolean, typ. false */


void pdf_set_page_number (pdf_page_handle pdf_page, char *page_number);

/* Create a new bookmark, under the specified parent, or at the top
   level if parent is NULL. */
pdf_bookmark_handle pdf_new_bookmark (pdf_bookmark_handle parent,
				      char *title,
				      bool open,
				      pdf_page_handle pdf_page);

void pdf_insert_tiff_image (pdf_page_handle pdf_page, char *filename);
