/*
 * tumble: build a PDF file from image files
 *
 * PDF routines
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
 */


#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitblt.h"
#include "pdf_util.h"


void pdf_fatal (char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  exit (2);
}


void *pdf_calloc (size_t nmemb, size_t size)
{
  void *m = calloc (nmemb, size);
  if (! m)
    pdf_fatal ("failed to allocate memory\n");
  return (m);
}


char *pdf_strdup (char *s)
{
  unsigned long len = strlen (s);
  char *s2 = pdf_calloc (1, len + 1);
  strcpy (s2, s);
  return (s2);
}
