/*
 * tiffg4: reencode a bilevel TIFF file as a single-strip TIFF Class F Group 4
 * Main program
 * $Id: tumble.c,v 1.3 2001/12/29 20:16:46 eric Exp $
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
#include "parser.tab.h"
#include "tiff2pdf.h"


int line;

FILE *yyin;
TIFF *in;
panda_pdf *out;


boolean close_tiff_input_file (void)
{
  if (in)
    TIFFClose (in);
  in = NULL;
  return (1);
}

boolean open_tiff_input_file (char *name)
{
  if (in)
    close_tiff_input_file ();
  in = TIFFOpen (name, "r");
  if (! in)
    {
      fprintf (stderr, "can't open input file '%s'\n", name);
      return (0);
    }
  return (1);
}


boolean close_pdf_output_file (void)
{
  if (out)
    panda_close (out);
  out = NULL;
  return (1);
}

boolean open_pdf_output_file (char *name)
{
  if (out)
    close_pdf_output_file ();
  out = panda_open (name, "w");
  if (! out)
    {
      return (0);
    }
  return (1);
}


boolean process_page (int image)  /* range 1 .. n */
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
  TIFFSetField (out, TIFFTAG_IMAGELENGTH, image_length);
  TIFFSetField (out, TIFFTAG_IMAGEWIDTH, image_width);
  TIFFSetField (out, TIFFTAG_PLANARCONFIG, planar_config);

  TIFFSetField (out, TIFFTAG_ROWSPERSTRIP, image_length);

  TIFFSetField (out, TIFFTAG_RESOLUTIONUNIT, resolution_unit);
  TIFFSetField (out, TIFFTAG_XRESOLUTION, x_resolution);
  TIFFSetField (out, TIFFTAG_YRESOLUTION, y_resolution);

  TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
  TIFFSetField (out, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
  TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
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
      TIFFWriteScanline (out, buffer, row, 0);
#endif
    }

  _TIFFfree (buffer);

  return (1);

 fail:
  return (0);
}


void input_images (int first, int last)
{
  if (first == last)
    printf ("image %d\n", first);
  else
    printf ("iamges %d..%d\n", first, last);
}

void output_pages (int first, int last)
{
  if (first == last)
    printf ("page %d\n", first);
  else
    printf ("pages %d..%d\n", first, last);
}


void yyerror (char *s)
{
  fprintf (stderr, "%d: %s\n", line, s);
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

  yyin = fopen (argv [1], "r");
  if (! yyin)
    {
      fprintf (stderr, "can't open spec file '%s'\n", argv [1]);
      result = 3;
      goto fail;
    }

  line = 1;

  yyparse ();

 fail:
  if (yyin)
    fclose (yyin);
  close_tiff_input_file ();
  close_pdf_output_file ();
  return (result);
}
