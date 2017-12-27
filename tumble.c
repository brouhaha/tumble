/*
 * tumble: build a PDF file from image files
 *
 * Main program
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
 *  2009-03-02 [JDB] Add support for overlay images and color key masking.
 *  2014-02-18 [JDB] Add -V option to print the program version.
 */


#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "semantics.h"

#ifdef CTL_LANG
#include "parser.tab.h"
#endif

#include "tumble.h"
#include "bitblt.h"
#include "pdf.h"
#include "tumble_input.h"


#define MAX_INPUT_FILES 5000

typedef struct output_file_t
{
  struct output_file_t *next;
  char *name;
  pdf_file_handle pdf;
} output_file_t;


int verbose, version;


output_file_t *output_files;
output_file_t *out;


char *progname;


bool close_pdf_output_files (void);


#define QMAKESTR(x) #x
#define MAKESTR(x) QMAKESTR(x)


void usage (void)
{
  fprintf (stderr, "\n");
  fprintf (stderr, "tumble version " MAKESTR(TUMBLE_VERSION) " - Copyright 2001-2017 Eric Smith <spacewar@gmail.com>\n");
  fprintf (stderr, "https://github.com/brouhaha/tumble/\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "usage:\n");
#ifdef CTL_LANG
  fprintf (stderr, "    %s [options] -c <control.tum>\n", progname);
#endif
  fprintf (stderr, "    %s [options] <input.tif>... -o <output.pdf>\n", progname);
  fprintf (stderr, "    %s -V\n", progname);
  fprintf (stderr, "options:\n");
  fprintf (stderr, "    -v        verbose\n");
  fprintf (stderr, "    -b <fmt>  create bookmarks\n");
  fprintf (stderr, "    -V        print program version\n");
  fprintf (stderr, "bookmark format:\n");
  fprintf (stderr, "    %%F  file name (sans suffix)\n");
  fprintf (stderr, "    %%p  page number\n");
}


/* generate fatal error message to stderr, doesn't return */
void fatal (int ret, char *format, ...) __attribute__ ((noreturn));

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
  close_input_file ();
  close_pdf_output_files ();
  exit (ret);
}


bool close_pdf_output_files (void)
{
  output_file_t *o, *n;

  for (o = output_files; o; o = n)
    {
      n = o->next;
      pdf_close (o->pdf, PDF_PAGE_MODE_USE_OUTLINES);
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

  o->pdf = pdf_create (name);
  if (! o->pdf)
    {
      fprintf (stderr, "can't open output file '%s'\n", name);
      free (o->name);
      free (o);
      return (0);
    }

  if (attributes->author)
    pdf_set_author (o->pdf, attributes->author);
  if (attributes->creator)
    pdf_set_creator (o->pdf, attributes->creator);
  if (attributes->title)
    pdf_set_title (o->pdf, attributes->title);
  if (attributes->subject)
    pdf_set_subject (o->pdf, attributes->subject);
  if (attributes->keywords)
    pdf_set_keywords (o->pdf, attributes->keywords);

  /* prepend new output file onto list */
  o->next = output_files;
  output_files = o;

  out = o;
  return (1);
}


#define MAX_BOOKMARK_LEVEL 20
static pdf_bookmark_handle bookmark_vector [MAX_BOOKMARK_LEVEL + 1] = { NULL };

static pdf_page_handle last_page = NULL;
static page_size_t last_size;

bool process_page (int image,  /* range 1 .. n */
		   input_attributes_t input_attributes,
		   bookmark_t *bookmarks,
		   page_label_t *page_label,
		   overlay_t *overlay,
		   rgb_range_t *transparency)
{
  pdf_page_handle page;
  image_info_t image_info;
  position_t position;
  
  if (! get_image_info (image, input_attributes, & image_info))
    return (0);

  if (overlay)
    {
      page = last_page;
      position.x = overlay->left * POINTS_PER_INCH;
      position.y = last_size.height - image_info.height_points - overlay->top * POINTS_PER_INCH;

      if (verbose)
	fprintf (stderr, "overlaying image at %.3f, %.3f\n", position.x, position.y);

    if (transparency)
      {
	input_attributes.transparency = transparency;
      }
    }
  else
    {
      last_page = page = pdf_new_page (out->pdf,
				       image_info.width_points,
				       image_info.height_points);
      last_size.width = image_info.width_points;
      last_size.height = image_info.height_points;
      position.x = 0.0;
      position.y = 0.0;
    }

  if (! process_image (image, input_attributes, & image_info, page, position))
    return (0);

  if (overlay)
    return (page != NULL);

  while (bookmarks)
    {
      if (bookmarks->level <= MAX_BOOKMARK_LEVEL)
	{
	  pdf_bookmark_handle parent = bookmark_vector [bookmarks->level - 1];
	  bookmark_vector [bookmarks->level] = pdf_new_bookmark (parent,
								 bookmarks->name,
								 0,
								 page);
	}
      else
	{
	  (void) pdf_new_bookmark (bookmark_vector [MAX_BOOKMARK_LEVEL],
				   bookmarks->name,
				   0,
				   page);
	}
      bookmarks = bookmarks->next;
    }

  if (page_label)
    pdf_new_page_label (out->pdf,
			page_label->page_index,
			page_label->base,
			page_label->count,
			page_label->style,
			page_label->prefix);

  return (page != NULL);
}


#define MAX_BOOKMARK_NAME_LEN 500


static int filename_length_without_suffix (char *in_fn)
{
  char *p;
  int len = strlen (in_fn);

  p = strrchr (in_fn, '.');
  if (p && match_input_suffix (p))
    return (p - in_fn);
  return (len);
}


/* $$$ this function should ensure that it doesn't overflow the name string! */
static void generate_bookmark_name (char *name,
				    char *bookmark_fmt, 
				    char *in_fn,
				    int page)
{
  bool meta = 0;
  int len;

  while (*bookmark_fmt)
    {
      if (meta)
	{
	  meta = 0;
	  switch (*bookmark_fmt)
	    {
	    case '%':
	      *(name++) = '%';
	      break;
	    case 'F':
	      len = filename_length_without_suffix (in_fn);
	      strncpy (name, in_fn, len);
	      name += len;
	      break;
	    case 'p':
	      sprintf (name, "%d", page);
	      name += strlen (name);
	      break;
	    default:
	      break;
	    }
	}
      else
	switch (*bookmark_fmt)
	  {
	  case '%':
	    meta = 1;
	    break;
	  default:
	    *(name++) = *bookmark_fmt;
	  }
      bookmark_fmt++;
    }
  *name = '\0';
}


void main_args (char *out_fn,
		int inf_count,
		char **in_fn,
		char *bookmark_fmt)
{
  int i, ip;
  input_attributes_t input_attributes;
  pdf_file_attributes_t output_attributes;
  bookmark_t bookmark;
  char bookmark_name [MAX_BOOKMARK_NAME_LEN];

  bookmark.next = NULL;
  bookmark.level = 1;
  bookmark.name = & bookmark_name [0];

  memset (& input_attributes, 0, sizeof (input_attributes));
  memset (& output_attributes, 0, sizeof (output_attributes));

  if (! open_pdf_output_file (out_fn, & output_attributes))
    fatal (3, "error opening output file \"%s\"\n", out_fn);
  for (i = 0; i < inf_count; i++)
    {
      if (! open_input_file (in_fn [i]))
	fatal (3, "error opening input file \"%s\"\n", in_fn [i]);
      for (ip = 1;; ip++)
	{
	  fprintf (stderr, "processing page %d of file \"%s\"\r", ip, in_fn [i]);
	  if (bookmark_fmt)
	    generate_bookmark_name (& bookmark_name [0],
				    bookmark_fmt, 
				    in_fn [i],
				    ip);
	  if (! process_page (ip, input_attributes,
			      bookmark_fmt ? & bookmark : NULL,
			      NULL,
			      NULL,
			      NULL))
	    fatal (3, "error processing page %d of input file \"%s\"\n", ip, in_fn [i]);
	  if (last_input_page ())
	    break;
	}
      if (verbose)
	fprintf (stderr, "processed %d pages of input file \"%s\"\n", ip, in_fn [i]);
      if (! close_input_file ())
	fatal (3, "error closing input file \"%s\"\n", in_fn [i]);
    }
  if (! close_pdf_output_files ())
    fatal (3, "error closing output file \"%s\"\n", out_fn);
}


#ifdef CTL_LANG
void main_control (char *control_fn)
{
  if (! parse_control_file (control_fn))
    fatal (2, "error parsing control file\n");
  if (! process_controls ())
    fatal (3, "error processing control file\n");
}
#endif


int main (int argc, char *argv[])
{
#ifdef CTL_LANG
  char *control_fn = NULL;
#endif
  char *out_fn = NULL;
  char *bookmark_fmt = NULL;
  int inf_count = 0;
  char *in_fn [MAX_INPUT_FILES];

  progname = argv [0];

  pdf_init ();

  init_tiff_handler ();
  init_jpeg_handler ();
  init_pbm_handler ();
  init_png_handler ();

  while (--argc)
    {
      if (argv [1][0] == '-')
	{
	  if (strcmp (argv [1], "-V") == 0)
	    {
	      version++;
	      break;
	    }
	  else if (strcmp (argv [1], "-v") == 0)
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
#ifdef CTL_LANG
	  else if (strcmp (argv [1], "-c") == 0)
	    {
	      if (argc)
		{
		  argc--;
		  argv++;
		  control_fn = argv [1];
		}
	      else
		fatal (1, "missing filename after \"-s\" option\n");
	    }
#endif
	  else if (strcmp (argv [1], "-b") == 0)
	    {
	      if (argc)
		{
		  argc--;
		  argv++;
		  bookmark_fmt = argv [1];
		}
	      else
		fatal (1, "missing format string after \"-b\" option\n");
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

  if (version)
    {
      puts (PDF_PRODUCER);
      exit (0);
    }

#ifdef CTL_LANG
  if (! ((! out_fn) ^ (! control_fn)))
    fatal (1, "either a control file or an output file (but not both) must be specified\n");
  if (control_fn && inf_count)
    fatal (1, "if control file is provided, input files can't be specified as arguments\n");
#else
  if (! out_fn)
    fatal (1, "an output file must be specified\n");
#endif

  if (out_fn && ! inf_count)
    fatal (1, "no input files specified\n");

#ifdef CTL_LANG
  if (control_fn)
    main_control (control_fn);
  else
    main_args (out_fn, inf_count, in_fn, bookmark_fmt);
#else
  main_args (out_fn, inf_count, in_fn, bookmark_fmt);
#endif
  
  close_input_file ();
  close_pdf_output_files ();
  exit (0);
}
