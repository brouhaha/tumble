#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "type.h"
#include "bitblt.h"


#define DIV_ROUND_UP(count,pow2) (((count) - 1) / (pow2) + 1)


static const u8 bit_reverse_byte [0x100] =
{
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static word_type bit_reverse_word (word_type d)
{
  return (bit_reverse_byte [d >> 24] |
	  (bit_reverse_byte [(d >> 16) & 0xff] << 8) |
	  (bit_reverse_byte [(d >> 8) & 0xff] << 16) |
	  (bit_reverse_byte [d & 0xff] << 24));
}


static u32 *temp_buffer;
static u32 temp_buffer_size;

static void realloc_temp_buffer (u32 size)
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


static inline word_type pixel_mask (x)
{
#ifdef LSB_LEFT
  return (1 << x);
#else
  return (1 << ((BITS_PER_WORD - 1) - x));
#endif
};

Bitmap *create_bitmap (Rect *rect)
{
  Bitmap *bitmap;
  u32 width = rect_width (rect);
  u32 height = rect_height (rect);

  if ((width <= 0) || (height <= 0))
    return (NULL);

  bitmap = calloc (1, sizeof (Bitmap));
  if (! bitmap)
    return (NULL);
  bitmap->rect = * rect;
  bitmap->row_words = DIV_ROUND_UP (width, BITS_PER_WORD);
  bitmap->bits = calloc (1, height * bitmap->row_words * sizeof (word_type));
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

boolean get_pixel (Bitmap *bitmap, Point coord)
{
  word_type *p;
  if ((coord.x < bitmap->rect.min.x) ||
      (coord.x >= bitmap->rect.max.x) ||
      (coord.y < bitmap->rect.min.y) ||
      (coord.y >= bitmap->rect.max.y))
    return (0);
  p = bitmap->bits +
    (coord.y - bitmap->rect.min.y) * bitmap->row_words +
    (coord.x - bitmap->rect.min.x) / BITS_PER_WORD;
  return ((*p & pixel_mask (coord.x & (BITS_PER_WORD - 1))) != 0);
}

void set_pixel (Bitmap *bitmap, Point coord, boolean value)
{
  word_type *p;
  if ((coord.x < bitmap->rect.min.x) ||
      (coord.x >= bitmap->rect.max.x) ||
      (coord.y < bitmap->rect.min.y) ||
      (coord.y >= bitmap->rect.max.y))
    return;
  p = bitmap->bits +
    (coord.y - bitmap->rect.min.y) * bitmap->row_words +
    (coord.x - bitmap->rect.min.x) / BITS_PER_WORD;
  if (value)
    *p |= pixel_mask (coord.x & (BITS_PER_WORD - 1));
  else
    *p &= ~pixel_mask (coord.x & (BITS_PER_WORD - 1));
}


/* modifies rect1 to be the intersection of rect1 and rect2;
   returns true if intersection is non-null */
static boolean clip_rect (Rect *rect1, Rect *rect2)
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


#if 0
static void blt_background (Bitmap *dest_bitmap,
			    Rect *dest_rect)
{
  s32 y;
  word_type *rp;

  /* This function requires a non-null dest rect */
  assert (dest_rect->min.x < dest_rect->max.x);
  assert (dest_rect->min.y < dest_rect->max.y);
 
  /* and that the rows of the dest rect lie entirely within the dest bitmap */
  assert (dest_rect->min.y >= dest_bitmap->rect->min.y);
  assert (dest_rect->max.y <= dest_bitmap->rect->max.y);

  /* clip the x axis of the dest_rect to the bounds of the dest bitmap */
  if (dest_rect->min.x < dest_bitmap->rect.min.x)
    dest_rect->min.x = dest_bitmap->rect.min.x;
  if (dest_rect->max.x > dest_bitmap->rect.max.x)
    dest_rect->max.x = dest_bitmap->rect.max.x;

  rp = ???;
  for (y = 0; y < rect_height (dest_rect); y++)
    {
  ???;
      rp += dest_bitmap->row_words;
    }
}


static void blt (Bitmap *src_bitmap,
		 Rect *src_rect,
		 Bitmap *dest_bitmap,
		 Rect *dest_rect)
{
  s32 y;
  word_type *rp;

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


Bitmap *bitblt (Bitmap *src_bitmap,
		Rect   *src_rect,
		Bitmap *dest_bitmap,
		Point  *dest_min,
		int tfn,
		int background)
{
  Rect sr, dr;     /* src and dest rects, clipped to visible portion of
		      dest rect */
  u32 drw, drh;    /* dest rect width, height - gets adjusted */
  Point src_point, dest_point;

  {
    sr = * src_rect;

    u32 srw = rect_width (& sr);
    u32 srh = rect_height (& sr);

    if ((srw < 0) || (srh < 0))
      goto done;  /* the source rect is empty! */

    dr.min = * dest_min;
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

  /* if the source rect min y is >= the source bitmap max y,
     we transfer background color to the entire dest rect */
  if (sr.min.y >= src->rect.max.y)
    {
      blt_background (dest_bitmap, & dr);
      goto done;
    }

  /* if the source rect min y is less than the source bitmap min y,
     we need to transfer some backgound color to the top part of the dest
     rect */
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

  /* now blt the available rows of the source rect */

  /* now transfer the background color to any remaining rows of the
     dest rect */
  if (??? )
    {
      blt_background (dest_bitmap, & dr);
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

  for (src_point.y = src_rect->min.y;
       src_point.y < src_rect->max.y;
       src_point.y++)
    {
      dest_point.y = dest_min->y + src_point.y - src_rect->min.y;

      for (src_point.x = src_rect->min.x;
	   src_point.x < src_rect->max.x;
	   src_point.x++)
	{
	  boolean a, b, c;

	  dest_point.x = dest_min->x + src_point.x - src_rect->min.x;

	  a = get_pixel (src_bitmap, src_point);
	  b = get_pixel (dest_bitmap, dest_point);
	  c = (tfn & (1 << (a * 2 + b))) != 0;

	  set_pixel (dest_bitmap, dest_point, c);
	}
    }
  return (dest_bitmap);
}
#endif


/* in-place transformations */
void flip_h (Bitmap *src)
{
  word_type *rp;  /* row pointer */
  word_type *p1;  /* work src ptr */
  word_type *p2;  /* work dest ptr */
  s32 y;
  int shift1, shift2;

  realloc_temp_buffer ((src->row_words + 1) * sizeof (word_type));

  rp = src->bits;
  if ((rect_width (& src->rect) & 7) == 0)
    {
      for (y = src->rect.min.y; y < src->rect.max.y; y++)
	{
	  memcpy (temp_buffer, rp, src->row_words * sizeof (word_type));
	  p1 = temp_buffer + src->row_words;
	  p2 = rp;

	  while (p1 >= temp_buffer)
	    *(p2++) = bit_reverse_word (*(p1--));
      
	  rp += src->row_words;
	}
      return;
    }

  temp_buffer [0] = 0;
  shift1 = rect_width (& src->rect) & (BITS_PER_WORD - 1);
  shift2 = BITS_PER_WORD - shift1;

  for (y = src->rect.min.y; y < src->rect.max.y; y++)
    {
      word_type d1, d2;

      memcpy (temp_buffer + 1, rp, src->row_words * sizeof (word_type));
      p1 = temp_buffer + src->row_words;
      p2 = rp;

      d2 = *(p1--);

      while (p1 >= temp_buffer)
	{
	  d1 = *(p1--);
	  *(p2++) = bit_reverse_word ((d1 << shift1) | (d2 >> shift2));
	  d2 = d1;
	}      

      rp += src->row_words;
    }
}

void flip_v (Bitmap *src)
{
  word_type *p1, *p2;

  realloc_temp_buffer (src->row_words * sizeof (word_type));

  p1 = src->bits;
  p2 = src->bits + src->row_words * (rect_height (& src->rect) - 1);
  while (p1 < p2)
    {
      memcpy (temp_buffer, p1, src->row_words * sizeof (word_type));
      memcpy (p1, p2, src->row_words * sizeof (word_type));
      memcpy (p2, temp_buffer, src->row_words * sizeof (word_type));
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
  u32 new_row_words = DIV_ROUND_UP (rect_height (& src->rect), 32);
  word_type *new_bits;

  new_bits = calloc (1, new_row_words * rect_width (& src->rect) * sizeof (word_type));

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
