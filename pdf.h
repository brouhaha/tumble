/*
 * tumble: build a PDF file from image files
 *
 * PDF routines
 * $Id: pdf.h,v 1.13 2003/03/19 23:53:09 eric Exp $
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
 *  2007-06-28 [JDB] Increase page limits from 45" to 200" square.
 *  2010-09-02 [JDB] Added support for min-is-black TIFF images.
 *  2014-02-18 [JDB] Added PDF_PRODUCER definition.
 */

#if !defined(SEMANTICS)
typedef struct
{
  int first; 
  int last;
 } range_t;

typedef struct
{
  int red;
  int green;
  int blue;
} rgb_t;

typedef struct
{
  range_t red;
  range_t green;
  range_t blue;
} rgb_range_t;

typedef struct
{
  rgb_t black_map;
  rgb_t white_map;
} colormap_t;
#endif

/* Acrobat default units aren't really points, but they're close. */
#define POINTS_PER_INCH 72.0

/* Page size for Acrobat Reader 4.0 and later is 200 x 200 inches.
   Old limit of 45 inches applied to Acrobat Reader 3.x only. */
#define PAGE_MAX_INCHES 200
#define PAGE_MAX_POINTS (PAGE_MAX_INCHES * POINTS_PER_INCH)


typedef struct pdf_file *pdf_file_handle;

typedef struct pdf_page *pdf_page_handle;

typedef struct pdf_bookmark *pdf_bookmark_handle;


#define PDF_PAGE_MODE_USE_NONE     0
#define PDF_PAGE_MODE_USE_OUTLINES 1  /* if no outlines, will use NONE */
#define PDF_PAGE_MODE_USE_THUMBS   2  /* not yet implemented */


void pdf_init (void);

pdf_file_handle pdf_create (char *filename);

void pdf_close (pdf_file_handle pdf_file, int page_mode);

#define AS_STR(S) #S
#define TO_STR(S) AS_STR(S)

#define PDF_PRODUCER "tumble " TO_STR(TUMBLE_VERSION) \
                     " by Eric Smith (modified by J. David Bryan)"

void pdf_set_author   (pdf_file_handle pdf_file, char *author);
void pdf_set_creator  (pdf_file_handle pdf_file, char *creator);
void pdf_set_producer (pdf_file_handle pdf_file, char *producer);
void pdf_set_title    (pdf_file_handle pdf_file, char *title);
void pdf_set_subject  (pdf_file_handle pdf_file, char *subject);
void pdf_set_keywords (pdf_file_handle pdf_file, char *keywords);


/* width and height in units of 1/72 inch */
pdf_page_handle pdf_new_page (pdf_file_handle pdf_file,
			      double width,
			      double height);

void pdf_close_page (pdf_page_handle pdf_page);


void pdf_write_text (pdf_page_handle pdf_page);


/* The length of the data must be Rows * rowbytes.
   Note that rowbytes must be at least (Columns+7)/8, but may be arbitrarily
   large. */
void pdf_write_g4_fax_image (pdf_page_handle pdf_page,
			     double x,
			     double y,
			     double width,
			     double height,
			     bool negative,
			     Bitmap *bitmap,
			     colormap_t *colormap,
			     rgb_range_t *transparency);


void pdf_write_jpeg_image (pdf_page_handle pdf_page,
			   double x,
			   double y,
			   double width,
			   double height,
			   bool color,
			   uint32_t width_samples,
			   uint32_t height_samples,
			   rgb_range_t *transparency,
			   FILE *f);


void pdf_write_png_image (pdf_page_handle pdf_page,
						  double x,
						  double y,
						  double width,
						  double height,
						  int color,
						  char *pal,
						  int palent,
						  int bpp,
						  uint32_t width_samples,
						  uint32_t height_samples,
						  rgb_range_t *transparency,
						  FILE *f);


void pdf_set_page_number (pdf_page_handle pdf_page, char *page_number);

/* Create a new bookmark, under the specified parent, or at the top
   level if parent is NULL. */
pdf_bookmark_handle pdf_new_bookmark (pdf_bookmark_handle parent,
				      char *title,
				      bool open,
				      pdf_page_handle pdf_page);


void pdf_new_page_label (pdf_file_handle pdf_file,
			 int page_index,
			 int base,
			 int count,
			 char style,
			 char *prefix);
