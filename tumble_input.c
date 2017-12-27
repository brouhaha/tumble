/*
 * tumble: build a PDF file from image files
 *
 * Input handler dispatch
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
 *  2007-05-07 [JDB] Add support for blank pages and fix a bug that caused a
 *                   double free.
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "semantics.h"
#include "tumble.h"
#include "bitblt.h"
#include "pdf.h"
#include "tumble_input.h"


#define MAX_INPUT_HANDLERS 10

static int input_handler_count = 0;

static input_handler_t *input_handlers [MAX_INPUT_HANDLERS];


static char *in_filename;
static FILE *in;
static input_handler_t *current_input_handler;


void install_input_handler (input_handler_t *handler)
{
  if (input_handler_count >= MAX_INPUT_HANDLERS)
    fprintf (stderr, "Too many input handlers, table only has room for %d\n", MAX_INPUT_HANDLERS);
  else
    input_handlers [input_handler_count++] = handler;
}


bool match_input_suffix (char *suffix)
{
  int i;
  for (i = 0; i < input_handler_count; i++)
    if (input_handlers [i]->match_suffix (suffix))
      return (1);
  return (0);
}


bool open_input_file (char *name)
{
  int i;

  if (name == NULL)
    {
      if (in)
	close_input_file ();
      current_input_handler = & blank_handler;
      return (1);
    }

  if (in)
    {
      if (strcmp (name, in_filename) == 0)
	return (1);
      close_input_file ();
    }
  in_filename = strdup (name);
  if (! in_filename)
    {
      fprintf (stderr, "can't strdup input filename '%s'\n", name);
      goto fail;
    }

  in = fopen (name, "rb");
  if (! in)
    goto fail;

  for (i = 0; i < input_handler_count; i++)
    {
      if (input_handlers [i]->open_input_file (in, name))
	break;
    }
  if (i >= input_handler_count)
    {
      fprintf (stderr, "unrecognized format for input file '%s'\n", name);
      goto fail;
    }
  current_input_handler = input_handlers [i];
  return (1);

 fail:
  if (in)
    fclose (in);
  in = NULL;
  return (0);
}


bool close_input_file (void)
{
  bool result = 1;

  if (current_input_handler)
    {
      result = current_input_handler->close_input_file ();
      current_input_handler = NULL;
    }
  if (in_filename) {
    free (in_filename);
    in_filename = NULL;
  }
  if (in)
    {
      fclose (in);
      in = NULL;
    }

  return (result);
}


bool last_input_page (void)
{
  if (! current_input_handler)
    return (0);
  return (current_input_handler->last_input_page ());
}


bool get_image_info (int image,
		     input_attributes_t input_attributes,
		     image_info_t *image_info)
{
  if (! current_input_handler)
    return (0);
  return (current_input_handler->get_image_info (image,
						 input_attributes,
						 image_info));
}

bool process_image (int image,
		    input_attributes_t input_attributes,
		    image_info_t *image_info,
		    pdf_page_handle page,
		    position_t position)
{
  if (! current_input_handler)
    return (0);
  return (current_input_handler->process_image (image,
						input_attributes,
						image_info,
						page,
						position));
}
