/*
 * tumble: build a PDF file from image files
 *
 * $Id: tumble_input.h,v 1.2 2003/03/20 06:55:27 eric Exp $
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


typedef struct
{
  bool color;
  uint32_t width_samples, height_samples;
  double width_points, height_points;
  double x_resolution, y_resolution;
} image_info_t;


typedef struct
{
  bool (*match_suffix) (char *suffix);
  bool (*open_input_file) (FILE *f, char *name);
  bool (*close_input_file) (void);
  bool (*last_input_page) (void);
  bool (*get_image_info) (int image,
			 input_attributes_t input_attributes,
			 image_info_t *image_info);
  bool (*process_image) (int image,
			 input_attributes_t input_attributes,
			 image_info_t *image_info,
			 pdf_page_handle page);
} input_handler_t;


void install_input_handler (input_handler_t *handler);


bool match_input_suffix (char *suffix);
bool open_input_file (char *name);
bool close_input_file (void);
bool last_input_page (void);
bool get_image_info (int image,
		     input_attributes_t input_attributes,
		     image_info_t *image_info);
bool process_image (int image,
		    input_attributes_t input_attributes,
		    image_info_t *image_info,
		    pdf_page_handle page);


void init_tiff_handler (void);
void init_jpeg_handler (void);
