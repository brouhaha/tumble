/*
 * tumble: build a PDF file from image files
 *
 * bitblt routines
 * $Id: bitblt.c,v 1.17 2003/03/16 07:27:06 eric Exp $
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */


#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitblt.h"

#include "bitblt_tables.h"


#define DIV_ROUND_UP(count,pow2) (((count) - 1) / (pow2) + 1)


void reverse_bits (uint8_t *p, int byte_count)
{
  while (byte_count--)
    {
      (*p) = bit_reverse_byte [*p];
      p++;
    }
}


static word_t bit_reverse_word (word_t d)
{
  return (bit_reverse_byte [d >> 24] |
	  (bit_reverse_byte [(d >> 16) & 0xff] << 8) |
	  (bit_reverse_byte [(d >> 8) & 0xff] << 16) |
	  (bit_reverse_byte [d & 0xff] << 24));
}


static void reverse_range_of_bytes (uint8_t *b, uint32_t count)
{
  uint8_t *b2 = b + count - 1;
  
  while (b < b2)
    {
      uint8_t t = bit_reverse_byte [*b];
      *b = bit_reverse_byte [*b2];
      *b2 = t;
      b++;
      b2--;
    }

  if (b == b2)
    *b = bit_reverse_byte [*b];
}


static word_t *temp_buffer;
static word_t temp_buffer_size;

static void realloc_temp_buffer (uint32_t size)
{
  if (size <= temp_buffer_size)
    return;
  temp_buffer = realloc (temp_buffer, size);
  if (! temp_buffer)
    {
      fprintf (stderr, "realloc failed in bitblt library\n");
      exit (2);
    }
  temp_buffer_size = size;
}


static inline word_t pixel_mask (int x)
{
#if defined (MIXED_ENDIAN)  /* disgusting hack for mixed-endian */
  word_t m;
  m = 0x80 >> (x & 7);
  m <<= (x & 24);
  return (m);
#elif defined (LSB_RIGHT)
  return (1U << ((BITS_PER_WORD - 1) - x));
#else
  return (1U << x);
#endif
};


/* mask for range of bits left..right, inclusive */
static inline word_t pixel_range_mask (int left, int right)
{
  word_t m1, m2, val;

  /* $$$ one of these cases is wrong! */
#if defined (LSB_RIGHT)
  m1 = (~ 0U) >> left;
  m2 = (~ 0U) << (BITS_PER_WORD - 1 - right);
#else
  m1 = (~ 0U) << left;
  m2 = (~ 0U) >> (BITS_PER_WORD - 1 - right);
#endif
  val = m1 & m2;

  printf ("left %d, right %d, mask %08x\n", left, right, val);
  return (val);
};


Bitmap *create_bitmap (Rect *rect)
{
  Bitmap *bitmap;
  uint32_t width = rect_width (rect);
  uint32_t height = rect_height (rect);

  if ((width <= 0) || (height <= 0))
    return (NULL);

  bitmap = calloc (1, sizeof (Bitmap));
  if (! bitmap)
    return (NULL);
  bitmap->rect = * rect;
  bitmap->row_words = DIV_ROUND_UP (width, BITS_PER_WORD);
  bitmap->bits = calloc (1, height * bitmap->row_words * sizeof (word_t));
  if (! bitmap->bits)
    {
      free (bitmap);
      return (NULL);
    }
  return (bitmap);
}

void free_bitmap (Bitmap *bitmap)
{
  free (bitmap->bits);
  free (bitmap);
}

bool get_pixel (Bitmap *bitmap, Point coord)
{
  word_t *p;
  int w,b;

  if ((coord.x < bitmap->rect.min.x) ||
      (coord.x >= bitmap->rect.max.x) ||
      (coord.y < bitmap->rect.min.y) ||
      (coord.y >= bitmap->rect.max.y))
    return (0);
  coord.y -= bitmap->rect.min.y;
  coord.x -= bitmap->rect.min.x;
  w = coord.x / BITS_PER_WORD;
  b = coord.x & (BITS_PER_WORD - 1);
  p = bitmap->bits + coord.y * bitmap->row_words + w;
  return (((*p) & pixel_mask (b)) != 0);
}

void set_pixel (Bitmap *bitmap, Point coord, bool value)
{
  word_t *p;
  int w,b;

  if ((coord.x < bitmap->rect.min.x) ||
      (coord.x >= bitmap->rect.max.x) ||
      (coord.y < bitmap->rect.min.y) ||
      (coord.y >= bitmap->rect.max.y))
    return;
  coord.y -= bitmap->rect.min.y;
  coord.x -= bitmap->rect.min.x;
  w = coord.x / BITS_PER_WORD;
  b = coord.x & (BITS_PER_WORD - 1);
  p = bitmap->bits + coord.y * bitmap->row_words + w;
  if (value)
    (*p) |= pixel_mask (b);
  else
    (*p) &= ~pixel_mask (b);
}


/* modifies rect1 to be the intersection of rect1 and rect2;
   returns true if intersection is non-null */
static bool clip_rect (Rect *rect1, Rect *rect2)
{
  if (rect1->min.y > rect2->max.y)
    goto empty;
  if (rect1->min.y < rect2->min.y)
    {
      if (rect1->max.y < rect2->max.y)
	goto empty;
      rect1->min.y = rect2->min.y;
    }
  if (rect1->max.y > rect2->max.y)
    rect1->max.y = rect2->max.y;

  if (rect1->min.x > rect2->max.x)
    goto empty;
  if (rect1->min.x < rect2->min.x)
    {
      if (rect1->max.x < rect2->max.x)
	goto empty;
      rect1->min.x = rect2->min.x;
    }
  if (rect1->max.x > rect2->max.x)
    rect1->max.x = rect2->max.x;

 empty:
  rect1->min.x = rect1->min.y =
    rect1->max.x = rect1->max.y = 0;
  return (0);
}


static void blt_background (Bitmap *dest_bitmap,
			    Rect dest_rect)
{
  uint32_t y;
  word_t *rp;
  uint32_t left_bit, left_word;
  uint32_t right_bit, right_word;
  word_t left_mask, right_mask;
  int32_t word_count;

  /* This function requires a non-null dest rect */
  assert (dest_rect.min.x < dest_rect.max.x);
  assert (dest_rect.min.y < dest_rect.max.y);
 
  /* and that the rows of the dest rect lie entirely within the dest bitmap */
  assert (dest_rect.min.y >= dest_bitmap->rect.min.y);
  assert (dest_rect.max.y <= dest_bitmap->rect.max.y);

  /* clip the x axis of the dest_rect to the bounds of the dest bitmap */
  if (dest_rect.min.x < dest_bitmap->rect.min.x)
    dest_rect.min.x = dest_bitmap->rect.min.x;
  if (dest_rect.max.x > dest_bitmap->rect.max.x)
    dest_rect.max.x = dest_bitmap->rect.max.x;

  rp = dest_bitmap->bits +
    (dest_rect.min.y - dest_bitmap->rect.min.y) * dest_bitmap->row_words +
    (dest_rect.min.x - dest_bitmap->rect.min.x) / BITS_PER_WORD;

  left_bit = dest_rect.min.x % BITS_PER_WORD;
  left_word = dest_rect.min.x / BITS_PER_WORD;

  right_bit = (dest_rect.max.x - 1) % BITS_PER_WORD;
  right_word = (dest_rect.max.x - 1) / BITS_PER_WORD;

  word_count = right_word + 1 - left_word;

  /* special case if entire horizontal range fits in a single word */
  if (word_count == 1)
    {
      left_mask = 0;
      right_mask = ~ pixel_range_mask (left_bit, right_bit);
      word_count = 0;
    }
  else
    {
      if (left_bit)
	{
	  left_mask = ~ pixel_range_mask (left_bit, BITS_PER_WORD - 1);
	  word_count--;
	}

      if (right_bit != (BITS_PER_WORD - 1))
	{
	  right_mask = ~ pixel_range_mask (0, right_bit);
	  word_count--;
	}
    }

  for (y = 0; y < rect_height (& dest_rect); y++)
    {
      word_t *wp = rp;

      /* partial word at left, if any */
      if (left_mask)
	*(wp++) &= left_mask;

      /* use Duff's Device for the full words */
      if (word_count)
	{
	  int32_t i = word_count;
	  switch (i % 8)
	    {
	      while (i > 0)
		{
		  *(wp++) = 0;
		case 7: *(wp++) = 0;
		case 6: *(wp++) = 0;
		case 5: *(wp++) = 0;
		case 4: *(wp++) = 0;
		case 3: *(wp++) = 0;
		case 2: *(wp++) = 0;
		case 1: *(wp++) = 0;
		case 0: i -= 8;
		}
	    }
	}

      /* partial word at right, if any */
      if (right_mask)
	*wp &= right_mask;

      /* advance to next row */
      rp += dest_bitmap->row_words;
    }
}


#if 0
static void blt (Bitmap *src_bitmap,
		 Rect *src_rect,
		 Bitmap *dest_bitmap,
		 Rect *dest_rect)
{
  int32_t y;
  word_t *rp;

  /* This function requires a non-null src rect */
  assert (dest_rect->min.x < dest_rect->max.x);
  assert (dest_rect->min.y < dest_rect->max.y);
 
  /* and a non-null dest rect */
  assert (dest_rect->min.x < dest_rect->max.x);
  assert (dest_rect->min.y < dest_rect->max.y);

  /* and that the widths and heights of the rects match */
  assert (rect_width (src_rect) == rect_width (dest_rect));
  assert (rect_height (src_rect) == rect_height (dest_rect));
 
  /* and that the rows of the src rect lie entirely within the src bitmap */
  assert (dest_rect->min.y >= dest_bitmap->rect->min.y);
  assert (dest_rect->max.y <= dest_bitmap->rect->max.y);

  /* and that the rows of the dest rect lie entirely within the dest bitmap */
  assert (dest_rect->min.y >= dest_bitmap->rect->min.y);
  assert (dest_rect->max.y <= dest_bitmap->rect->max.y);

  /* clip the x axis of the dest_rect to the bounds of the dest bitmap,
     and adjust the src_rect to match */
  if (dest_rect->min.x < dest_bitmap->rect.min.x)
    {
      src_rect->min.x += ???;
      dest_rect->min.x = dest_bitmap->rect.min.x;
    }
  if (dest_rect->max.x > dest_bitmap->rect.max.x)
    {
      dest_rect->max.x = dest_bitmap->rect.max.x;
    }

  rp = ???;
  for (y = 0; y < rect_height (dest_rect); y++)
    {
  ???;
      rp += dest_bitmap->row_words;
    }
}


/*
 * The destination rectangle is first clipped to the dest bitmap, and
 * the source rectangle is adjusted in the corresponding manner.
 * What's left is divided into five sections, any of which may be
 * null.  The portion that actually corresponds to the intersection of
 * the source rectangle and the source bitmpa is the "middle".  The
 * other four sections will use the background color as the source
 * operand.
 *
 *          
 *   y0 ->  -------------------------------------------------
 *          |                     top                       |
 *          |                                               |
 *   y1 ->  -------------------------------------------------
 *          |   left        |    middle     |    right      |
 *          |               |               |               |
 *   y2 ->  -------------------------------------------------
 *          |                     bottom                    |
 *          |                                               |
 *   y3 ->  -------------------------------------------------
 *
 *          ^               ^               ^               ^
 *          |               |               |               |
 *         x0              x1              x2              x3
 *
 * */
Bitmap *bitblt (Bitmap *src_bitmap,
		Rect   *src_rect,
		Bitmap *dest_bitmap,
		Point  *dest_min,
		int tfn,
		int background)
{
  Rect sr, dr;     /* src and dest rects, clipped to visible portion of
		      dest rect */
  uint32_t drw, drh;    /* dest rect width, height - gets adjusted */
  Point src_point, dest_point;

  /* dest coordinates: */
  uint32_t x0, x1, x2, x3;
  uint32_t y0, y1, y2, y3;

  {
    sr = * src_rect;

    uint32_t srw = rect_width (& sr);
    uint32_t srh = rect_height (& sr);

    if ((srw < 0) || (srh < 0))
      goto done;  /* the source rect is empty! */

    dr.min.x = dest_min->x;
    dr.min.y = dest_min->y;
    dr.max.x = dr.min.x + srw;
    dr.max.y = dr.min.y + srh;
  }

  if (! dest_bitmap)
    {
      dest_bitmap = create_bitmap (& dr);
      if (! dest_bitmap)
	return (NULL);
    }

  if ((dr.min.x >= dest_bitmap->rect.max.x) ||
      (dr.min.y >= dest_bitmap->rect.max.y))
    goto done;  /* the dest rect isn't even in the dest bitmap! */

  /* crop dest rect to dest bitmap */
  delta = dest_bitmap->rect.min.x - dr.min.x;
  if (delta > 0)
    {
      sr.min.x += delta;
      dr.min.x += delta;
    }

  delta = dest_bitmap->rect.min.y - dr.min.y;
  if (delta > 0)
    {
      sr.min.y += delta;
      dr.min.y += delta;
    }

  delta = dr.max.x - dest_bitmap->rect.max.x;
  if (delta > 0)
    {
      sr.max.x -= delta;
      dr.max.x -= delta;
    }

  delta = dr.max.y - dest_bitmap->rect.max.y;
  if (delta > 0)
    {
      sr.max.x -= delta;
      dr.max.x -= delta;
    }

  drw = rect_width (& dr);
  drh = rect_height (& dh);

  x0 = dr.min.x;
  y0 = dr.min.y;
  x3 = dr.max.x;
  y3 = dr.max.y;

#if 0
  /* if the source rect min y is >= the source bitmap max y,
     we transfer background color to the entire dest rect */
  if (sr.min.y >= src->rect.max.y)
    {
      blt_background (dest_bitmap, dr);
      goto done;
    }
#endif

  /* top */
  if (y0 != y1)
    {
      dr2.min.x = x0;
      dr2.max.x = x3;
      dr2.min.y = y0;
      dr2.max.y = y1;
      blt_background (dest_bitmap, & dr2);
    }

  /*
   * top:  if the source rect min y is less than the source bitmap min y,
   * we need to transfer some backgound color to the top part of the dest
   * rect
   */
  if (sr.min.y < src->rect.min.y)
    {
      Rect dr2;
      uint32 bg_height;

      bg_height = src->rect.min.y - sr.min.y;
      if (bg_height > sh)
	bg_height = sh;

      dr2 = dr;
      dr2.max.y = dr2.min.y + bg_height;

      blt_background (dest_bitmap, & dr2);

      /* now reduce the rect height by the number of lines of background
	 color */
      sr.min.y += bg_height;
      dr.min.y += bg_height;
      sh -= bg_height;
      dh -= bg_height;

      if (sr.min.y == sr.max.y)
	goto done;
    }

  if (y1 != y2)
    {
      /* left */
      if (x0 != x1)
	{
	  dr2.min.x = x1;
	  dr2.max.x = x1;
	  dr2.min.y = y1;
	  dr2.max.y = y2
	  blt_background (dest_bitmap, & dr2);
	}

      /* middle */
      if (x1 != x2)
	{
	  /* ??? */
	}

      /* right */
      if (x2 != x3)
	{
	  dr2.min.x = x2;
	  dr2.max.x = x3;
	  dr2.min.y = y1;
	  dr2.max.y = y2
	  blt_background (dest_bitmap, & dr2);
	}
    }

  /* bottom */
  if (y2 != y3)
    {
      dr2.min.x = x0;
      dr2.max.x = x3;
      dr2.min.y = y2;
      dr2.max.y = y3;
      blt_background (dest_bitmap, & dr2);
    }

 done:
  return (dest_bitmap);
}
#else
Bitmap *bitblt (Bitmap *src_bitmap,
		Rect   *src_rect,
		Bitmap *dest_bitmap,
		Point  *dest_min,
		int tfn,
		int background)
{
  Point src_point, dest_point;

  if (! dest_bitmap)
    {
      Rect dest_rect = {{ 0, 0 }, { dest_min->x + rect_width (src_rect),
				    dest_min->y + rect_height (src_rect) }};
      dest_bitmap = create_bitmap (& dest_rect);
      if (! dest_bitmap)
	return (NULL);
    }

  if (tfn == TF_SRC)
    {
      for (src_point.y = src_rect->min.y;
	   src_point.y < src_rect->max.y;
	   src_point.y++)
	{
	  dest_point.y = dest_min->y + src_point.y - src_rect->min.y;
	  
	  for (src_point.x = src_rect->min.x;
	       src_point.x < src_rect->max.x;
	       src_point.x++)
	    {
	      bool a;

	      dest_point.x = dest_min->x + src_point.x - src_rect->min.x;

	      a = get_pixel (src_bitmap, src_point);
	      set_pixel (dest_bitmap, dest_point, a);
	    }
	}
    }
  else
    {
      for (src_point.y = src_rect->min.y;
	   src_point.y < src_rect->max.y;
	   src_point.y++)
	{
	  dest_point.y = dest_min->y + src_point.y - src_rect->min.y;
	  
	  for (src_point.x = src_rect->min.x;
	       src_point.x < src_rect->max.x;
	       src_point.x++)
	    {
	      bool a, b, c;

	      dest_point.x = dest_min->x + src_point.x - src_rect->min.x;

	      a = get_pixel (src_bitmap, src_point);
	      b = get_pixel (dest_bitmap, dest_point);
	      c = (tfn & (1 << (a * 2 + b))) != 0;

	      set_pixel (dest_bitmap, dest_point, c);
	    }
	}
    }
  return (dest_bitmap);
}
#endif


/* in-place transformations */
void flip_h (Bitmap *src)
{
  word_t *rp;  /* row pointer */
  int32_t y;
  int shift1, shift2;

  rp = src->bits;
  if ((rect_width (& src->rect) & 7) == 0)
    {
      for (y = src->rect.min.y; y < src->rect.max.y; y++)
	{
          reverse_range_of_bytes ((uint8_t *) rp, rect_width (& src->rect) / 8);
	  rp += src->row_words;
	}
      return;
    }

  realloc_temp_buffer ((src->row_words + 1) * sizeof (word_t));

  temp_buffer [0] = 0;
  shift1 = rect_width (& src->rect) & (BITS_PER_WORD - 1);
  shift2 = BITS_PER_WORD - shift1;

  for (y = src->rect.min.y; y < src->rect.max.y; y++)
    {
      word_t d1, d2;
      word_t *p1;  /* work src ptr */
      word_t *p2;  /* work dest ptr */

      memcpy (temp_buffer + 1, rp, src->row_words * sizeof (word_t));
      p1 = temp_buffer + src->row_words;
      p2 = rp;

      d2 = *(p1--);

      while (p1 >= temp_buffer)
	{
	  word_t t;
	  d1 = *(p1--);
	  t = (d1 >> shift1) | (d2 << shift2);
	  *(p2++) = bit_reverse_word (t);
	  d2 = d1;
	}      

      rp += src->row_words;
    }
}


void flip_v (Bitmap *src)
{
  word_t *p1, *p2;

  realloc_temp_buffer (src->row_words * sizeof (word_t));

  p1 = src->bits;
  p2 = src->bits + src->row_words * (rect_height (& src->rect) - 1);
  while (p1 < p2)
    {
      memcpy (temp_buffer, p1, src->row_words * sizeof (word_t));
      memcpy (p1, p2, src->row_words * sizeof (word_t));
      memcpy (p2, temp_buffer, src->row_words * sizeof (word_t));
      p1 += src->row_words;
      p2 -= src->row_words;
    }
}

void rot_180 (Bitmap *src)  /* combination of flip_h and flip_v */
{
  flip_h (src);
  flip_v (src);
}

/* "in-place" transformations - will allocate new memory and free old */
void transpose (Bitmap *src)
{
  uint32_t new_row_words = DIV_ROUND_UP (rect_height (& src->rect), 32);
  word_t *new_bits;

  new_bits = calloc (1, new_row_words * rect_width (& src->rect) * sizeof (word_t));

  /* $$$ more code needed here */
}

void rot_90 (Bitmap *src)   /* transpose + flip_h */
{
  transpose (src);
  flip_h (src);
}

void rot_270 (Bitmap *src)  /* transpose + flip_v */
{
  transpose (src);
  flip_v (src);
}


