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


#include <assert.h>


void pdf_fatal (char *fmt, ...) __attribute__ ((noreturn));

void *pdf_calloc (size_t nmemb, size_t size);

char *pdf_strdup (char *s);

#if 1
#define pdf_assert(cond) assert(cond)
#else
#define pdf_assert(cond) do \
    { \
      if (! (cond)) \
        pdf_fatal ("assert at %s(%d)\n", __FILE__, __LINE__); \
    } while (0)
#endif
