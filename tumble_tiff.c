/*
 * tumble: build a PDF file from image files
 *
 * $Id: tumble_tiff.c,v 1.5 2003/03/20 07:22:23 eric Exp $
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
 *  2010-09-02 [JDB] Added support for min-is-black TIFF images.
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>  /* strcasecmp() is a BSDism */

#include <tiffio.h>
/*
 * On the x86, libtiff defaults to bit-endian bit order for no good reason.
 * In theory, the '-L' (and maybe '-H') should give us little-endian bit
 * order, but it doesn't seem to work.  Thus we reverse the bits ourselves
 * after we read in the file.
 */
#define TIFF_REVERSE_BITS


#include "semantics.h"
#include "tumble.h"
#include "bitblt.h"
#include "pdf.h"
#include "tumble_input.h"


TIFF *tiff_in;


#define SWAP(type,a,b) do { type temp; temp = a; a = b; b = temp; } while (0)


static bool match_tiff_suffix (char *suffix)
{
  return ((strcasecmp (suffix, ".tif") == 0) ||
	  (strcasecmp (suffix, ".tiff") == 0));
}


static bool close_tiff_input_file (void)
{
  TIFFClose (tiff_in);
  return (1);
}


static bool open_tiff_input_file (FILE *f, char *name)
{
  uint8_t buf [2];
  size_t l;

  l = fread (& buf [0], 1, sizeof (buf), f);
  if (l != sizeof (buf))
    return (0);

  rewind (f);

  if (! (((buf [0] == 0x49) && (buf [1] == 0x49)) ||
	 ((buf [0] == 0x4d) && (buf [1] == 0x4d))))
    return (0);

  tiff_in = TIFFFdOpen (fileno (f), name, "r");
  if (! tiff_in)
    {
      fprintf (stderr, "can't open input file '%s'\n", name);
      return (0);
    }
  return (1);
}


static bool last_tiff_input_page (void)
{
  return (TIFFLastDirectory (tiff_in));
}


static bool get_tiff_image_info (int image,
				 input_attributes_t input_attributes,
				 image_info_t *image_info)
{
  uint32_t image_height, image_width;
  uint16_t samples_per_pixel;
  uint16_t bits_per_sample;
  uint16_t photometric_interpretation;
  uint16_t planar_config;

  uint16_t resolution_unit;
  float x_resolution, y_resolution;

  double dest_x_resolution, dest_y_resolution;

#ifdef CHECK_DEPTH
  uint32_t image_depth;
#endif

  if (! TIFFSetDirectory (tiff_in, image - 1))
    {
      fprintf (stderr, "can't find page %d of input file\n", image);
      return (0);
    }
  if (1 != TIFFGetField (tiff_in, TIFFTAG_IMAGELENGTH, & image_height))
    {
      fprintf (stderr, "can't get image height\n");
      return (0);
    }
  if (1 != TIFFGetField (tiff_in, TIFFTAG_IMAGEWIDTH, & image_width))
    {
      fprintf (stderr, "can't get image width\n");
      return (0);
    }

  if (1 != TIFFGetField (tiff_in, TIFFTAG_SAMPLESPERPIXEL, & samples_per_pixel))
    {
      fprintf (stderr, "can't get samples per pixel\n");
      return (0);
    }

#ifdef CHECK_DEPTH
  if (1 != TIFFGetField (tiff_in, TIFFTAG_IMAGEDEPTH, & image_depth))
    {
      fprintf (stderr, "can't get image depth\n");
      return (0);
    }
#endif

  if (1 != TIFFGetField (tiff_in, TIFFTAG_BITSPERSAMPLE, & bits_per_sample))
    {
      fprintf (stderr, "can't get bits per sample\n");
      return (0);
    }

  if (1 != TIFFGetField (tiff_in, TIFFTAG_PHOTOMETRIC, & photometric_interpretation))
    {
      fprintf(stderr, "warning: photometric interpretation not specified, assuming min-is-white\n");
      photometric_interpretation = PHOTOMETRIC_MINISWHITE;
    }

  else if ((photometric_interpretation != PHOTOMETRIC_MINISWHITE) &&
	   (photometric_interpretation != PHOTOMETRIC_MINISBLACK))
    {
      fprintf(stderr, "photometric interpretation value %u is invalid\n", photometric_interpretation);
      return (0);
    }

  if (1 != TIFFGetField (tiff_in, TIFFTAG_PLANARCONFIG, & planar_config))
    planar_config = 1;

  if (1 != TIFFGetField (tiff_in, TIFFTAG_RESOLUTIONUNIT, & resolution_unit))
    resolution_unit = 2;
  if (1 != TIFFGetField (tiff_in, TIFFTAG_XRESOLUTION, & x_resolution))
    x_resolution = 300;
  if (1 != TIFFGetField (tiff_in, TIFFTAG_YRESOLUTION, & y_resolution))
    y_resolution = 300;

  if (samples_per_pixel != 1)
    {
      fprintf (stderr, "samples per pixel %u, must be 1\n", samples_per_pixel);
      return (0);
    }

#ifdef CHECK_DEPTH
  if (image_depth != 1)
    {
      fprintf (stderr, "image depth %u, must be 1\n", image_depth);
      return (0);
    }
#endif

  if (bits_per_sample != 1)
    {
      fprintf (stderr, "bits per sample %u, must be 1\n", bits_per_sample);
      return (0);
    }

  if (planar_config != 1)
    {
      fprintf (stderr, "planar config %u, must be 1\n", planar_config);
      return (0);
    }

  if (input_attributes.has_resolution)
    {
      x_resolution = input_attributes.x_resolution;
      y_resolution = input_attributes.y_resolution;
    }

  if ((input_attributes.rotation == 90) || (input_attributes.rotation == 270))
    {
      image_info->width_samples  = image_height;
      image_info->height_samples = image_width;
      dest_x_resolution = y_resolution;
      dest_y_resolution = x_resolution;
      SWAP (double, image_info->width_points, image_info->height_points);
    }
  else
    {
      image_info->width_samples = image_width;
      image_info->height_samples = image_height;
      dest_x_resolution = x_resolution;
      dest_y_resolution = y_resolution;
    }

  image_info->width_points = (image_info->width_samples / dest_x_resolution) * POINTS_PER_INCH;
  image_info->height_points = (image_info->height_samples / dest_y_resolution) * POINTS_PER_INCH;

  if ((image_info->height_points > PAGE_MAX_POINTS) || 
      (image_info->width_points > PAGE_MAX_POINTS))
    {
      fprintf (stdout, "image too large (max %d inches on a side\n", PAGE_MAX_INCHES);
      return (0);
    }

  image_info->negative = (photometric_interpretation == PHOTOMETRIC_MINISBLACK);

  return (1);
}




static bool process_tiff_image (int image,  /* range 1 .. n */
				input_attributes_t input_attributes,
				image_info_t *image_info,
				pdf_page_handle page,
				position_t position)
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
    if (1 != TIFFReadScanline (tiff_in,
			       bitmap->bits + row * bitmap->row_words,
			       row,
			       0))
      {
	fprintf (stderr, "can't read TIFF scanline\n");
	goto fail;
      }

#ifdef TIFF_REVERSE_BITS
  reverse_bits ((uint8_t *) bitmap->bits,
		rect.max.y * bitmap->row_words * sizeof (word_t));
#endif /* TIFF_REVERSE_BITS */

#if 0
  if (input_attributes.has_page_size)
    bitmap = resize_bitmap (bitmap,
			    input_attributes.page_size.width * x_resolution,
			    input_attributes.page_size.height * y_resolution);
#endif

  rotate_bitmap (bitmap, input_attributes.rotation);

#if 0
  pdf_write_text (page);
#else
  pdf_write_g4_fax_image (page,
			  position.x, position.y,
			  image_info->width_points, image_info->height_points,
			  image_info->negative,
			  bitmap,
			  input_attributes.colormap,
			  input_attributes.transparency);
#endif

  result = 1;

 fail:
  if (bitmap)
    free_bitmap (bitmap);
  return (result);
}


input_handler_t tiff_handler =
  {
    match_tiff_suffix,
    open_tiff_input_file,
    close_tiff_input_file,
    last_tiff_input_page,
    get_tiff_image_info,
    process_tiff_image
  };


void init_tiff_handler (void)
{
  install_input_handler (& tiff_handler);
}
