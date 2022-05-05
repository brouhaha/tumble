/*
 * tumble: build a PDF file from image files
 *
 * Copyright 2001, 2002, 2003, 2017 Eric Smith <spacewar@gmail.com>
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
 *  2009-03-02 [JDB] Add support for overlay images.
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>  /* strcasecmp() is a BSDism */

#define HAVE_BOOL 1
#include <pbm.h>
/*
 * pbm_readpbmrow_packed always uses big-endian bit ordering.
 * On little-endian processors (such as the x86), we want little-endian
 * bit order, so we must reverse the bits ourselves after we read in the
 * file.
 */
#define PBM_REVERSE_BITS


#include "semantics.h"
#include "tumble.h"
#include "bitblt.h"
#include "pdf.h"
#include "tumble_input.h"


typedef struct
{
  FILE *f;
  int rows;
  int cols;
  int format;
} pbm_info_t;

static pbm_info_t pbm;


#define SWAP(type,a,b) do { type temp; temp = a; a = b; b = temp; } while (0)


static bool match_pbm_suffix (char *suffix)
{
  return strcasecmp (suffix, ".pbm") == 0;
}


static bool close_pbm_input_file (void)
{
  pbm.f = NULL;
  return true;
}


static bool open_pbm_input_file (FILE *f, char *name)
{
  uint8_t buf [2];
  size_t l;

  l = fread (& buf [0], 1, sizeof (buf), f);
  if (l != sizeof (buf))
    return false;

  rewind (f);

  if (! (((buf [0] == 'P') && (buf [1] == '1')) ||
	 ((buf [0] == 'P') && (buf [1] == '4'))))
    return false;

  pbm.f = f;

  pbm_readpbminit (f, & pbm.cols, & pbm.rows, & pbm.format);

  return true;
}


static bool last_pbm_input_page (void)
{
  /* only handle single-page PBM files for now */
  return true;
}


static bool get_pbm_image_info (int image,
				input_attributes_t input_attributes,
				image_info_t *image_info)
{
  double x_resolution = 300;
  double y_resolution = 300;
  double dest_x_resolution, dest_y_resolution;

  if (input_attributes.has_resolution)
    {
      x_resolution = input_attributes.x_resolution;
      y_resolution = input_attributes.y_resolution;
    }

  if ((input_attributes.rotation == 90) || (input_attributes.rotation == 270))
    {
      image_info->width_samples = pbm.rows;
      image_info->height_samples = pbm.cols;
      dest_x_resolution = y_resolution;
      dest_y_resolution = x_resolution;
    }
  else
    {
      image_info->width_samples = pbm.cols;
      image_info->height_samples = pbm.rows;
      dest_x_resolution = x_resolution;
      dest_y_resolution = y_resolution;
    }


  image_info->width_points = (image_info->width_samples / dest_x_resolution) * POINTS_PER_INCH;
  image_info->height_points = (image_info->height_samples / dest_y_resolution) * POINTS_PER_INCH;

  if ((image_info->height_points > PAGE_MAX_POINTS) || 
      (image_info->width_points > PAGE_MAX_POINTS))
    {
      fprintf (stdout, "image too large (max %d inches on a side\n", PAGE_MAX_INCHES);
      return false;
    }

  image_info->negative = false;
  
  return true;
}


static bool process_pbm_image (int image,  /* range 1 .. n */
			       input_attributes_t input_attributes,
			       image_info_t *image_info,
			       pdf_page_handle page,
			       output_attributes_t output_attributes)
{
  bool result = 0;
  Rect rect;
  Bitmap *bitmap = NULL;

  int row;

  rect.min.x = 0;
  rect.min.y = 0;

  if ((input_attributes.rotation == 90) || (input_attributes.rotation == 270))
    {
      rect.max.x = image_info->height_samples;
      rect.max.y = image_info->width_samples;
    }
  else
    {
      rect.max.x = image_info->width_samples;
      rect.max.y = image_info->height_samples;
    }

  bitmap = create_bitmap (& rect);

  if (! bitmap)
    {
      fprintf (stderr, "can't allocate bitmap\n");
      fprintf (stderr, "width %d height %d\n", image_info->width_samples, image_info->height_samples);
      goto fail;
    }

  for (row = 0; row < rect.max.y; row++)
    {
      pbm_readpbmrow_packed (pbm.f,
			     (unsigned char *) (bitmap->bits + row * bitmap->row_words),
			     pbm.cols,
			     pbm.format);
      }

#ifdef PBM_REVERSE_BITS
  reverse_bits ((uint8_t *) bitmap->bits,
		rect.max.y * bitmap->row_words * sizeof (word_t));
#endif /* PBM_REVERSE_BITS */

  /* $$$ need to invert bits here */

#if 0
  if (input_attributes.has_page_size)
    bitmap = resize_bitmap (bitmap,
			    x_resolution,
			    y_resolution,
			    input_attributes);
#endif

  rotate_bitmap (bitmap, input_attributes.rotation);

  pdf_write_g4_fax_image (page,
			  output_attributes.position.x, output_attributes.position.y,
			  image_info->width_points, image_info->height_points,
			  image_info->negative,
			  bitmap,
			  output_attributes.overlay,
			  output_attributes.colormap,
			  input_attributes.transparency);

  result = 1;

 fail:
  if (bitmap)
    free_bitmap (bitmap);
  return result;
}


input_handler_t pbm_handler =
  {
    match_pbm_suffix,
    open_pbm_input_file,
    close_pbm_input_file,
    last_pbm_input_page,
    get_pbm_image_info,
    process_pbm_image
  };


void init_pbm_handler (void)
{
  /* why should we let libpbm look at the real args? */
  int fake_argc = 1;
  char *fake_argv [] = { "tumble" };

  pbm_init (& fake_argc, fake_argv);
  install_input_handler (& pbm_handler);
}
