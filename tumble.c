/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * Main program
 * $Id: tumble.c,v 1.20 2003/01/21 10:30:49 eric Exp $
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA */


#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <tiffio.h>
#define TIFF_REVERSE_BITS

#include <panda/functions.h>
#include <panda/constants.h>

#include "bitblt.h"
#include "semantics.h"
#include "parser.tab.h"
#include "t2p.h"


#define MAX_INPUT_FILES 5000

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


int verbose;


char *in_filename;
TIFF *in;
output_file_t *output_files;
output_file_t *out;
/* panda_pdf *out; */


char *progname;


bool close_tiff_input_file (void);
bool close_pdf_output_files (void);


void usage (void)
{
  fprintf (stderr, "usage:\n");
  fprintf (stderr, "    %s [options] -s spec\n", progname);
  fprintf (stderr, "    %s [options] <input.tif>... -o <output.pdf>\n", progname);
  fprintf (stderr, "options:\n");
  fprintf (stderr, "    -v   verbose\n");
}


/* generate fatal error message to stderr, doesn't return */
void fatal (int ret, char *format, ...)
{
  va_list ap;

  fprintf (stderr, "fatal error");
  if (format)
    {
      fprintf (stderr, ": ");
      va_start (ap, format);
      vfprintf (stderr, format, ap);
      va_end (ap);
    }
  else
    fprintf (stderr, "\n");
  if (ret == 1)
    usage ();
  close_tiff_input_file ();
  close_pdf_output_files ();
  exit (ret);
}


bool close_tiff_input_file (void)
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


bool open_tiff_input_file (char *name)
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


bool close_pdf_output_files (void)
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

bool open_pdf_output_file (char *name,
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


/* frees original! */
static Bitmap *resize_bitmap (Bitmap *src,
			      float x_resolution,
			      float y_resolution,
			      input_attributes_t input_attributes)
{
  Rect src_rect;
  Point dest_min;
  Bitmap *dest;

  int width_pixels = input_attributes.page_size.width * x_resolution;
  int height_pixels = input_attributes.page_size.height * y_resolution;

  src_rect.min.x = (rect_width (& src->rect) - width_pixels) / 2;
  src_rect.min.y = (rect_height (& src->rect) - height_pixels) / 2;
  src_rect.max.x = src_rect.min.x + width_pixels;
  src_rect.max.y = src_rect.min.y + height_pixels;

  dest_min.x = 0;
  dest_min.y = 0;

  dest = bitblt (src, & src_rect, NULL, & dest_min, TF_SRC, 0);
  free_bitmap (src);
  return (dest);
}


/* "in place" rotation */
static void rotate_bitmap (Bitmap *src,
			   input_attributes_t input_attributes)
{
  switch (input_attributes.rotation)
    {
    case 0: break;
    case 90: rot_90 (src); break;
    case 180: rot_180 (src); break;
    case 270: rot_270 (src); break;
    default:
      fprintf (stderr, "rotation must be 0, 90, 180, or 270\n");
    }
}


#define SWAP(type,a,b) do { type temp; temp = a; a = b; b = temp; } while (0)


bool last_tiff_page (void)
{
  return (TIFFLastDirectory (in));
}


bool process_page (int image,  /* range 1 .. n */
		   input_attributes_t input_attributes,
		   bookmark_t *bookmarks)
{
  int result = 0;

  uint32_t image_length, image_width;
  uint32_t dest_image_length, dest_image_width;
#ifdef CHECK_DEPTH
  uint32_t image_depth;
#endif

  uint16_t samples_per_pixel;
  uint16_t bits_per_sample;
  uint16_t planar_config;

  uint16_t resolution_unit;
  float x_resolution, y_resolution;
  float dest_x_resolution, dest_y_resolution;

  int width_points, height_points;  /* really 1/72 inch units rather than
				       points */

  Rect rect;
  Bitmap *bitmap;

  int row;

  panda_page *page;

  int tiff_temp_fd;
  char tiff_temp_fn [] = "/var/tmp/t2p-XXXXXX\0";
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

  if (1 != TIFFGetField (in, TIFFTAG_RESOLUTIONUNIT, & resolution_unit))
    resolution_unit = 2;
  if (1 != TIFFGetField (in, TIFFTAG_XRESOLUTION, & x_resolution))
    x_resolution = 300;
  if (1 != TIFFGetField (in, TIFFTAG_YRESOLUTION, & y_resolution))
    y_resolution = 300;

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

  if (input_attributes.has_resolution)
    {
      x_resolution = input_attributes.x_resolution;
      y_resolution = input_attributes.y_resolution;
    }

  if ((input_attributes.rotation == 90) || (input_attributes.rotation == 270))
    {
      dest_image_width  = image_length;
      dest_image_length = image_width;
      dest_x_resolution = y_resolution;
      dest_y_resolution = x_resolution;
      SWAP (int, width_points, height_points);
    }
  else
    {
      dest_image_width = image_width;
      dest_image_length = image_length;
      dest_x_resolution = x_resolution;
      dest_y_resolution = y_resolution;
    }

  rect.min.x = 0;
  rect.min.y = 0;
  rect.max.x = image_width;
  rect.max.y = image_length;

  bitmap = create_bitmap (& rect);

  if (! bitmap)
    {
      fprintf (stderr, "can't allocate bitmap\n");
      goto fail;
    }

  for (row = 0; row < image_length; row++)
    if (1 != TIFFReadScanline (in,
			       bitmap->bits + row * bitmap->row_words,
			       row,
			       0))
      {
	fprintf (stderr, "can't read TIFF scanline\n");
	goto fail;
      }

#ifdef TIFF_REVERSE_BITS
  reverse_bits ((uint8_t *) bitmap->bits,
		image_length * bitmap->row_words * sizeof (word_type));
#endif /* TIFF_REVERSE_BITS */

  if (input_attributes.has_page_size)
    bitmap = resize_bitmap (bitmap,
			    x_resolution,
			    y_resolution,
			    input_attributes);

  rotate_bitmap (bitmap,
		 input_attributes);

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

  TIFFSetField (tiff_temp, TIFFTAG_IMAGELENGTH, rect_height (& bitmap->rect));
  TIFFSetField (tiff_temp, TIFFTAG_IMAGEWIDTH, rect_width (& bitmap->rect));
  TIFFSetField (tiff_temp, TIFFTAG_PLANARCONFIG, planar_config);

  TIFFSetField (tiff_temp, TIFFTAG_ROWSPERSTRIP, rect_height (& bitmap->rect));

  TIFFSetField (tiff_temp, TIFFTAG_RESOLUTIONUNIT, resolution_unit);
  TIFFSetField (tiff_temp, TIFFTAG_XRESOLUTION, dest_x_resolution);
  TIFFSetField (tiff_temp, TIFFTAG_YRESOLUTION, dest_y_resolution);

  TIFFSetField (tiff_temp, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel);
  TIFFSetField (tiff_temp, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
  TIFFSetField (tiff_temp, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
  TIFFSetField (tiff_temp, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);

#ifdef TIFF_REVERSE_BITS
  reverse_bits ((uint8_t *) bitmap->bits,
		image_length * bitmap->row_words * sizeof (word_type));
#endif /* TIFF_REVERSE_BITS */

  for (row = 0; row < rect_height (& bitmap->rect); row++)
    if (1 != TIFFWriteScanline (tiff_temp,
				bitmap->bits + row * bitmap->row_words,
				row,
				0))
      {
	fprintf (stderr, "can't write TIFF scanline\n");
	goto fail;
      }

  TIFFClose (tiff_temp);

  width_points = (rect_width (& bitmap->rect) / dest_x_resolution) * POINTS_PER_INCH;
  height_points = (rect_height (& bitmap->rect) / dest_y_resolution) * POINTS_PER_INCH;

  free_bitmap (bitmap);

  if ((height_points > PAGE_MAX_POINTS) || (width_points > PAGE_MAX_POINTS))
    {
      fprintf (stdout, "image too large (max %d inches on a side\n", PAGE_MAX_INCHES);
      goto fail;
    }

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


void main_args (char *out_fn, int inf_count, char **in_fn)
{
  int i, ip;
  input_attributes_t input_attributes;
  pdf_file_attributes_t output_attributes;

  memset (& input_attributes, 0, sizeof (input_attributes));
  memset (& output_attributes, 0, sizeof (output_attributes));

  if (! open_pdf_output_file (out_fn, & output_attributes))
    fatal (3, "error opening output file \"%s\"\n", out_fn);
  for (i = 0; i < inf_count; i++)
    {
      if (! open_tiff_input_file (in_fn [i]))
	fatal (3, "error opening input file \"%s\"\n", in_fn [i]);
      for (ip = 1;; ip++)
	{
	  if (! process_page (ip, input_attributes, NULL))
	    fatal (3, "error processing page %d of input file \"%s\"\n", ip, in_fn [i]);
	  if (last_tiff_page ())
	    break;
	}
      if (verbose)
	fprintf (stderr, "processed %d pages of input file \"%s\"\n", ip, in_fn [i]);
      if (! close_tiff_input_file ())
	fatal (3, "error closing input file \"%s\"\n", in_fn [i]);
    }
  if (! close_pdf_output_files ())
    fatal (3, "error closing output file \"%s\"\n", out_fn);
}


void main_spec (char *spec_fn)
{
  if (! parse_spec_file (spec_fn))
    fatal (2, "error parsing spec file\n");
  if (! process_specs ())
    fatal (3, "error processing spec file\n");
}


int main (int argc, char *argv[])
{
  char *spec_fn = NULL;
  char *out_fn = NULL;
  int inf_count = 0;
  char *in_fn [MAX_INPUT_FILES];

  progname = argv [0];

  panda_init ();

  while (--argc)
    {
      if (argv [1][0] == '-')
	{
	  if (strcmp (argv [1], "-v") == 0)
	    verbose++;
	  else if (strcmp (argv [1], "-o") == 0)
	    {
	      if (argc)
		{
		  argc--;
		  argv++;
		  out_fn = argv [1];
		}
	      else
		fatal (1, "missing filename after \"-o\" option\n");
	    }
	  else if (strcmp (argv [1], "-s") == 0)
	    {
	      if (argc)
		{
		  argc--;
		  argv++;
		  spec_fn = argv [1];
		}
	      else
		fatal (1, "missing filename after \"-s\" option\n");
	    }
	  else
	    fatal (1, "unrecognized option \"%s\"\n", argv [1]);
	}
      else if (inf_count < MAX_INPUT_FILES)
	in_fn [inf_count++] = argv [1];
      else
	fatal (1, "exceeded maximum of %d input files\n", MAX_INPUT_FILES);
      argv++;
    }

  if (! ((! out_fn) ^ (! spec_fn)))
    fatal (1, "either a spec file or an output file (but not both) must be specified\n");

  if (out_fn && ! inf_count)
    fatal (1, "no input files specified\n");

  if (spec_fn && inf_count)
    fatal (1, "if spec file is provided, input files can't be specified as arguments\n");

  if (spec_fn)
    main_spec (spec_fn);
  else
    main_args (out_fn, inf_count, in_fn);
  
  close_tiff_input_file ();
  close_pdf_output_files ();
  exit (0);
}
