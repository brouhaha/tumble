/*
 * tiffg4: reencode a bilevel TIFF file as a single-strip TIFF Class F Group 4
 * Main program
 * $Id: tumble.c,v 1.8 2001/12/31 19:44:40 eric Exp $
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
#include <tiffio.h>
#include <panda/functions.h>
#include <panda/constants.h>

#include "type.h"
#include "bitblt.h"
#include "semantics.h"
#include "parser.tab.h"
#include "tiff2pdf.h"


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

boolean open_pdf_output_file (char *name)
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
  if (! 0)
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
  u32 image_length, image_width;
#ifdef CHECK_DEPTH
  u32 image_depth;
#endif
  u16 bits_per_sample;
  u16 planar_config;
  u16 resolution_unit;
  float x_resolution, y_resolution;

  char *buffer;
  u32 row;

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

#if 0
  TIFFSetField (out->pdf, TIFFTAG_IMAGELENGTH, image_length);
  TIFFSetField (out->pdf, TIFFTAG_IMAGEWIDTH, image_width);
  TIFFSetField (out->pdf, TIFFTAG_PLANARCONFIG, planar_config);

  TIFFSetField (out->pdf, TIFFTAG_ROWSPERSTRIP, image_length);

  TIFFSetField (out->pdf, TIFFTAG_RESOLUTIONUNIT, resolution_unit);
  TIFFSetField (out->pdf, TIFFTAG_XRESOLUTION, x_resolution);
  TIFFSetField (out->pdf, TIFFTAG_YRESOLUTION, y_resolution);

  TIFFSetField (out->pdf, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
  TIFFSetField (out->pdf, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
  TIFFSetField (out->pdf, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
#endif

  buffer = _TIFFmalloc (TIFFScanlineSize (in));
  if (! buffer)
    {
      fprintf (stderr, "failed to allocate buffer\n");
      goto fail;
    }

  for (row = 0; row < image_length; row++)
    {
      TIFFReadScanline (in, buffer, row, 0);
#if 0
      TIFFWriteScanline (out->pdf, buffer, row, 0);
#endif
    }

  _TIFFfree (buffer);

  return (1);

 fail:
  return (0);
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
