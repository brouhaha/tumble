#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "type.h"
#include "bitblt.h"


#define CALC_ROWBYTES(width) (((width) - 1) / 8 + 1)


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


static u8 *temp_buffer;
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


static inline u8 pixel_mask (x)
{
#ifdef LSB_LEFT
  return (1 << x);
#else
  return (1 << (7 - x));
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
  bitmap->rowbytes = CALC_ROWBYTES (width);
  bitmap->bits = calloc (height * bitmap->rowbytes, 1);
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
  u8 *p;
  if ((coord.x < bitmap->rect.min.x) ||
      (coord.x >= bitmap->rect.max.x) ||
      (coord.y < bitmap->rect.min.y) ||
      (coord.y >= bitmap->rect.max.y))
    return (0);
  p = bitmap->bits +
    (coord.y - bitmap->rect.min.y) * bitmap->rowbytes +
    (coord.x - bitmap->rect.min.x) / 8;
  return ((*p & pixel_mask (coord.x & 7)) != 0);
}

void set_pixel (Bitmap *bitmap, Point coord, boolean value)
{
  u8 *p;
  if ((coord.x < bitmap->rect.min.x) ||
      (coord.x >= bitmap->rect.max.x) ||
      (coord.y < bitmap->rect.min.y) ||
      (coord.y >= bitmap->rect.max.y))
    return;
  p = bitmap->bits +
    (coord.y - bitmap->rect.min.y) * bitmap->rowbytes +
    (coord.x - bitmap->rect.min.x) / 8;
  if (value)
    *p |= pixel_mask (coord.x & 7);
  else
    *p &= (0xff ^ pixel_mask (coord.x & 7));
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


Bitmap *bitblt (Bitmap *src_bitmap,
		Rect   *src_rect,
		Bitmap *dest_bitmap,
		Point  *dest_min,
		int tfn)
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


/* in-place transformations */
void flip_h (Bitmap *src)
{
  u8 *rp;  /* row pointer */
  u8 *p1;  /* work src ptr */
  u8 *p2;  /* work dest ptr */
  u16 d;
  s32 y;
  int shift;

  realloc_temp_buffer (src->rowbytes + 1);

  rp = src->bits;
  if ((rect_width (& src->rect) & 7) == 0)
    {
      for (y = src->rect.min.y; y < src->rect.max.y; y++)
	{
	  memcpy (temp_buffer, rp, src->rowbytes);
	  p1 = temp_buffer + src->rowbytes - 1;
	  p2 = rp;

	  while (p1 >= temp_buffer)
	    *(p2++) = bit_reverse_byte [*(p1--)];
      
	  rp += src->rowbytes;
	}
      return;
    }

  temp_buffer [0] = 0;
  shift = 8 - (rect_width (& src->rect) & 7);

  for (y = src->rect.min.y; y < src->rect.max.y; y++)
    {
      memcpy (temp_buffer + 1, rp, src->rowbytes);
      p1 = temp_buffer + src->rowbytes;
      p2 = rp;

      d = *(p1--);

      while (p1 >= temp_buffer)
	{
	  d = (d >> 8) | ((*(p1--)) << 8);
	  *(p2++) = bit_reverse_byte [(d >> shift) & 0xff];
	}      

      rp += src->rowbytes;
    }
}

void flip_v (Bitmap *src)
{
  u8 *p1, *p2;

  realloc_temp_buffer (src->rowbytes);

  p1 = src->bits;
  p2 = src->bits + src->rowbytes * (rect_height (& src->rect) - 1);
  while (p1 < p2)
    {
      memcpy (temp_buffer, p1, src->rowbytes);
      memcpy (p1, p2, src->rowbytes);
      memcpy (p2, temp_buffer, src->rowbytes);
      p1 += src->rowbytes;
      p2 -= src->rowbytes;
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
  u32 new_rowbytes = CALC_ROWBYTES (rect_height (& src->rect));
  u8 *new_bits;

  new_bits = calloc (1, new_rowbytes * rect_width (& src->rect));

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
