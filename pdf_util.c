#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libpdf.h"
#include "libpdf_util.h"


void pdf_fatal (char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  exit (2);
}


void *pdf_calloc (long int size)
{
  void *m = calloc (1, size);
  if (! m)
    pdf_fatal ("failed to allocate memory\n");
  return (m);
}


char *pdf_strdup (char *s)
{
  unsigned long len = strlen (s);
  char *s2 = pdf_calloc (len + 1);
  strcpy (s2, s);
  return (s2);
}
