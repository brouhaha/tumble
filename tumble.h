/*
 * tumble: build a PDF file from image files
 *
 * $Id: tumble.h,v 1.17 2003/03/19 22:54:07 eric Exp $
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
 *  2009-03-02 [JDB] Add support for overlay images and color key masking.
 */


extern int verbose;


typedef struct
{
  bool has_resolution;
  double x_resolution;
  double y_resolution;

  bool has_page_size;
  page_size_t page_size;

  bool has_rotation;
  int rotation;

  bool has_crop;
  crop_t crop;

  rgb_range_t *transparency;

  colormap_t *colormap;		// really an output attribute, but we don't have such a thing

} input_attributes_t;


bool open_input_file (char *name);
bool close_input_file (void);


typedef struct
{
  char *author;
  char *creator;
  char *title;
  char *subject;
  char *keywords;
} pdf_file_attributes_t;

bool open_pdf_output_file (char *name,
			   pdf_file_attributes_t *attributes);


bool process_page (int image,  /* range 1 .. n */
		   input_attributes_t input_attributes,
		   bookmark_t *bookmarks,
		   page_label_t *page_label,
		   overlay_t *overlay,
		   rgb_range_t *transparency);
