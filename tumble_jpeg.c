/*
 * tumble: build a PDF file from image files
 *
 * $Id: tumble_jpeg.c,v 1.4 2003/03/20 06:55:27 eric Exp $
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
#include <strings.h>  /* strcasecmp() is a BSDism */
#include <jpeglib.h>


#include "semantics.h"
#include "tumble.h"
#include "bitblt.h"
#include "pdf.h"
#include "tumble_input.h"


static FILE *jpeg_f;

static struct jpeg_decompress_struct cinfo;
static struct jpeg_error_mgr jerr;


static bool match_jpeg_suffix (char *suffix)
{
  return ((strcasecmp (suffix, ".jpg") == 0) ||
	  (strcasecmp (suffix, ".jpeg") == 0));
}

static bool close_jpeg_input_file (void)
{
  return (1);
}


static bool open_jpeg_input_file (FILE *f, char *name)
{
  uint8_t buf [2];
  size_t l;

  l = fread (& buf [0], 1, sizeof (buf), f);
  if (l != sizeof (buf))
    return (0);

  rewind (f);

  if ((buf [0] != 0xff) || (buf [1] != 0xd8))
    return (0);

  cinfo.err = jpeg_std_error (& jerr);
  jpeg_create_decompress (& cinfo);

  jpeg_stdio_src (& cinfo, f);

  jpeg_read_header (& cinfo, TRUE);

  rewind (f);

  jpeg_f = f;

  return (1);
}


static bool last_jpeg_input_page (void)
{
  return (1);
}


static bool get_jpeg_image_info (int image,
				 input_attributes_t input_attributes,
				 image_info_t *image_info)
{
  double unit;

#ifdef DEBUG_JPEG
  printf ("color space: %d\n", cinfo.jpeg_color_space);
  printf ("components: %d\n", cinfo.num_components);
  printf ("density unit: %d\n", cinfo.density_unit);
  printf ("x density: %d\n", cinfo.X_density);
  printf ("y density: %d\n", cinfo.Y_density);
  printf ("width: %d\n", cinfo.image_width);
  printf ("height: %d\n", cinfo.image_height);
#endif

  switch (cinfo.jpeg_color_space)
    {
    case JCS_GRAYSCALE:
      if (cinfo.num_components != 1)
	{
	  fprintf (stderr, "JPEG grayscale image has %d components, should have 1\n",
		   cinfo.num_components);
	  return (0);
	}
      image_info->color = 0;
      break;
    case JCS_RGB:
    case JCS_YCbCr:
      if (cinfo.num_components != 3)
	{
	  fprintf (stderr, "JPEG RGB or YCbCr image has %d components, should have 3\n",
		   cinfo.num_components);
	  return (0);
	}
      image_info->color = 1;
      break;
    default:
      fprintf (stderr, "JPEG color space %d not supported\n", cinfo.jpeg_color_space);
      return (0);
    }
  image_info->width_samples = cinfo.image_width;
  image_info->height_samples = cinfo.image_height;

  if (cinfo.saw_JFIF_marker & cinfo.density_unit)
    {
      switch (cinfo.density_unit)
	{
	case 1:  /* samples per inch */
	  unit = 1.0;
	  break;
	case 2:  /* samples per cm */
	  unit = 2.54;
	  break;
	default:
	  fprintf (stderr, "JFIF density unit %d not supported\n", cinfo.density_unit);
	  return (0);
	}
      image_info->width_points = ((image_info->width_samples * POINTS_PER_INCH) /
				  (cinfo.X_density * unit));
      image_info->height_points = ((image_info->height_samples * POINTS_PER_INCH) /
				   (cinfo.Y_density * unit));
    }
  else
    {
      /* assume 300 DPI - not great, but what else can we do? */
      image_info->width_points = (image_info->width_samples * POINTS_PER_INCH) / 300.0;
      image_info->height_points = (image_info->height_samples * POINTS_PER_INCH) / 300.0;
    }

  return (1);
}


static bool process_jpeg_image (int image,  /* range 1 .. n */
				input_attributes_t input_attributes,
				image_info_t *image_info,
				pdf_page_handle page)
{
  pdf_write_jpeg_image (page,
			0, 0,  /* x, y */
			image_info->width_points,
			image_info->height_points,
			image_info->color,
			image_info->width_samples,
			image_info->height_samples,
			jpeg_f);

  return (1);
}


input_handler_t jpeg_handler =
  {
    match_jpeg_suffix,
    open_jpeg_input_file,
    close_jpeg_input_file,
    last_jpeg_input_page,
    get_jpeg_image_info,
    process_jpeg_image
  };


void init_jpeg_handler (void)
{
  install_input_handler (& jpeg_handler);
}
