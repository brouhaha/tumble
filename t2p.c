/*
 * tiff2pdf: Create a PDF file from the contents of one or more
 *           TIFF bilevel image files.  The images in the resulting
 *           PDF file will be compressed using ITU-T T.6 (G4) fax
 *           encoding.
 *
 * Main program
 * $Id: t2p.c,v 1.11 2001/12/31 22:11:43 eric Exp $
 * Copyright 2001 Eric Smith <eric@brouhaha.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <tiffio.h>
#include <panda/functions.h>
#include <panda/constants.h>

#include "type.h"
#include "bitblt.h"
#include "semantics.h"
#include "parser.tab.h"
#include "tiff2pdf.h"


#define POINTS_PER_INCH 72

/* page size limited by Acrobat Reader to 45 inches on a side */
#define PAGE_MAX_INCHES 45
#define PAGE_MAX_POINTS (PAGE_MAX_INCHES * POINTS_PER_INCH)


typedef struct output_file_t
{
  struct output_file_t *next;
  char *name;
  panda_pdf *pdf;
} output_file_t;


char *in_filename;
TIFF *in;
output_file_t *output_files;
output_file_t *out;
/* panda_pdf *out; */


boolean close_tiff_input_file (void)
{
  if (in)
    {
      free (in_filename);
      TIFFClose (in);
    }
  in = NULL;
  in_filename = NULL;
  return (1);
}

boolean open_tiff_input_file (char *name)
{
  if (in)
    {
      if (strcmp (name, in_filename) == 0)
	return (1);
      close_tiff_input_file ();
    }
  in_filename = strdup (name);
  if (! in_filename)
    {
      fprintf (stderr, "can't strdup input filename '%s'\n", name);
      return (0);
    }
  in = TIFFOpen (name, "r");
  if (! in)
    {
      fprintf (stderr, "can't open input file '%s'\n", name);
      free (in_filename);
      return (0);
    }
  return (1);
}


boolean close_pdf_output_files (void)
{
  output_file_t *o, *n;

  for (o = output_files; o; o = n)
    {
      n = o->next;
      panda_close (o->pdf);
      free (o->name);
      free (o);
    }
  out = NULL;
  output_files = NULL;
  return (1);
}

boolean open_pdf_output_file (char *name,
			      pdf_file_attributes_t *attributes)
{
  output_file_t *o;

  if (out && (strcmp (name, out->name) == 0))
    return (1);
  for (o = output_files; o; o = o->next)
    if (strcmp (name, o->name) == 0)
      {
	out = o;
	return (1);
      }
  o = calloc (1, sizeof (output_file_t));
  if (! o)
    {
      fprintf (stderr, "can't calloc output file struct for '%s'\n", name);
      return (0);
   }

  o->name = strdup (name);
  if (! o->name)
    {
      fprintf (stderr, "can't strdup output filename '%s'\n", name);
      free (o);
      return (0);
    }

  o->pdf = panda_open (name, "w");
  if (! o->pdf)
    {
      fprintf (stderr, "can't open output file '%s'\n", name);
      free (o->name);
      free (o);
      return (0);
    }

  if (attributes->author)
    panda_setauthor (o->pdf, attributes->author);
  if (attributes->creator)
    panda_setcreator (o->pdf, attributes->creator);
  if (attributes->title)
    panda_settitle (o->pdf, attributes->title);
  if (attributes->subject)
    panda_setsubject (o->pdf, attributes->subject);
  if (attributes->keywords)
    panda_setkeywords (o->pdf, attributes->keywords);

  /* prepend new output file onto list */
  o->next = output_files;
  output_files = o;

  out = o;
  return (1);
}


void process_page_numbers (int page_index,
			   int count,
			   int base,
			   page_label_t *page_label)
{
}


boolean process_page (int image,  /* range 1 .. n */
		      input_attributes_t input_attributes,
		      bookmark_t *bookmarks)
{
  int result = 0;

  u32 image_length, image_width;
#ifdef CHECK_DEPTH
  u32 image_depth;
#endif

  u16 samples_per_pixel;
  u16 bits_per_sample;
  u16 planar_config;
  u16 resolution_unit;
  float x_resolution, y_resolution;
  int width_points, height_points;  /* really 1/72 inch units rather than
				       points */


  char *buffer;
  u32 row;

  panda_page *page;

  int tiff_temp_fd;
  char tiff_temp_fn [] = "/var/tmp/tiff2pdf-XXXXXX\0";
  TIFF *tiff_temp;
  
  char pagesize [26];  /* Needs to hold two ints of four characters (0..3420),
			  two zeros, three spaces, two brackets, and a NULL.
                          Added an extra ten characters just in case. */

  if (! TIFFSetDirectory (in, image - 1))
    {
      fprintf (stderr, "can't find page %d of input file\n", image);
      goto fail;
    }
  if (1 != TIFFGetField (in, TIFFTAG_IMAGELENGTH, & image_length))
    {
      fprintf (stderr, "can't get image length\n");
      goto fail;
    }
  if (1 != TIFFGetField (in, TIFFTAG_IMAGEWIDTH, & image_width))
    {
      fprintf (stderr, "can't get image width\n");
      goto fail;
    }

  if (1 != TIFFGetField (in, TIFFTAG_SAMPLESPERPIXEL, & samples_per_pixel))
    {
      fprintf (stderr, "can't get samples per pixel\n");
      goto fail;
    }

#ifdef CHECK_DEPTH
  if (1 != TIFFGetField (in, TIFFTAG_IMAGEDEPTH, & image_depth))
    {
      fprintf (stderr, "can't get image depth\n");
      goto fail;
    }
#endif

  if (1 != TIFFGetField (in, TIFFTAG_BITSPERSAMPLE, & bits_per_sample))
    {
      fprintf (stderr, "can't get bits per sample\n");
      goto fail;
    }

  if (1 != TIFFGetField (in, TIFFTAG_PLANARCONFIG, & planar_config))
    planar_config = 1;

  printf ("image length %u width %u, "
#ifdef CHECK_DEPTH
          "depth %u, "
#endif
          "planar config %u\n",
	  image_length, image_width,
#ifdef CHECK_DEPTH
	  image_depth,
#endif
	  planar_config);

  if (1 != TIFFGetField (in, TIFFTAG_RESOLUTIONUNIT, & resolution_unit))
    resolution_unit = 2;
  if (1 != TIFFGetField (in, TIFFTAG_XRESOLUTION, & x_resolution))
    x_resolution = 300;
  if (1 != TIFFGetField (in, TIFFTAG_YRESOLUTION, & y_resolution))
    y_resolution = 300;

  printf ("resolution unit %u, x resolution %f, y resolution %f\n",
	  resolution_unit, x_resolution, y_resolution);

  if (samples_per_pixel != 1)
    {
      fprintf (stderr, "samples per pixel %u, must be 1\n", samples_per_pixel);
      goto fail;
    }

#ifdef CHECK_DEPTH
  if (image_depth != 1)
    {
      fprintf (stderr, "image depth %u, must be 1\n", image_depth);
      goto fail;
    }
#endif

  if (bits_per_sample != 1)
    {
      fprintf (stderr, "bits per sample %u, must be 1\n", bits_per_sample);
      goto fail;
    }

  if (planar_config != 1)
    {
      fprintf (stderr, "planar config %u, must be 1\n", planar_config);
      goto fail;
    }

  tiff_temp_fd = mkstemp (tiff_temp_fn);
  if (tiff_temp_fd < 0)
    {
      fprintf (stderr, "can't create temporary TIFF file\n");
      goto fail;
    }

  tiff_temp = TIFFFdOpen (tiff_temp_fd, tiff_temp_fn, "w");
  if (! out)
    {
      fprintf (stderr, "can't open temporary TIFF file '%s'\n", tiff_temp_fn);
      goto fail;
    }

  TIFFSetField (tiff_temp, TIFFTAG_IMAGELENGTH, image_length);
  TIFFSetField (tiff_temp, TIFFTAG_IMAGEWIDTH, image_width);
  TIFFSetField (tiff_temp, TIFFTAG_PLANARCONFIG, planar_config);

  TIFFSetField (tiff_temp, TIFFTAG_ROWSPERSTRIP, image_length);

  TIFFSetField (tiff_temp, TIFFTAG_RESOLUTIONUNIT, resolution_unit);
  TIFFSetField (tiff_temp, TIFFTAG_XRESOLUTION, x_resolution);
  TIFFSetField (tiff_temp, TIFFTAG_YRESOLUTION, y_resolution);

  TIFFSetField (tiff_temp, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel);
  TIFFSetField (tiff_temp, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
  TIFFSetField (tiff_temp, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
  TIFFSetField (tiff_temp, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);

  buffer = _TIFFmalloc (TIFFScanlineSize (in));
  if (! buffer)
    {
      fprintf (stderr, "failed to allocate buffer\n");
      goto fail;
    }

  for (row = 0; row < image_length; row++)
    {
      TIFFReadScanline (in, buffer, row, 0);
      TIFFWriteScanline (tiff_temp, buffer, row, 0);
    }

  _TIFFfree (buffer);
  TIFFClose (tiff_temp);

  width_points = (image_width / x_resolution) * POINTS_PER_INCH;
  height_points = (image_length / y_resolution) * POINTS_PER_INCH;

  if ((height_points > PAGE_MAX_POINTS) || (width_points > PAGE_MAX_POINTS))
    {
      fprintf (stdout, "image too large (max %d inches on a side\n", PAGE_MAX_INCHES);
      goto fail;
    }

  printf ("height_points %d, width_points %d\n", height_points, width_points);

  sprintf (pagesize, "[0 0 %d %d]", width_points, height_points);

  page = panda_newpage (out->pdf, pagesize);
  panda_imagebox (out->pdf,
		  page,
		  0, /* top */
		  0, /* left */
		  height_points, /* bottom */
		  width_points, /* right */
		  tiff_temp_fn,
		  panda_image_tiff);

  result = 1;

 fail:
  if (tiff_temp_fd)
    unlink (tiff_temp_fn);
  return (result);
}


int main (int argc, char *argv[])
{
  int result = 0;

  panda_init ();

  if (argc != 2)
    {
      fprintf (stderr, "usage: %s spec\n", argv [0]);
      result = 1;
      goto fail;
    }

  if (! parse_spec_file (argv [1]))
    {
      result = 2;
      goto fail;
    }

  if (! process_specs ())
    {
      result = 3;
      goto fail;
    }
  
 fail:
  close_tiff_input_file ();
  close_pdf_output_files ();
  return (result);
}
