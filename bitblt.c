#include <stdlib.h>

#include "bitblt.h"

static inline u8 pixel_mask (x)
{
#ifdef LSB_LEFT
  return (1 << x);
#else
  return (1 << (7 - x));
#endif
}

static inline u32 rect_width (Rect r)
{
  return (r.lower_right.x - r.upper_left.x);
}

static inline u32 rect_height (Rect r)
{
  return (r.lower_right.y - r.upper_left.y);
}

Bitmap *create_bitmap (u32 width, u32 height)
{
  Bitmap *bitmap;

  bitmap = calloc (1, sizeof (Bitmap));
  if (! bitmap)
    return (NULL);
  bitmap->width = width;
  bitmap->height = height;
  bitmap->rowbytes = (width - 1) / 8 + 1;
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
  if ((coord.x >= bitmap->width) || (coord.y >= bitmap->height))
    return (0);
  p = bitmap->bits + coord.y * bitmap->rowbytes + coord.x / 8;
  return ((*p & pixel_mask (coord.x & 7)) != 0);
}

void set_pixel (Bitmap *bitmap, Point coord, boolean value)
{
  u8 *p;
  if ((coord.x >= bitmap->width) || (coord.y >= bitmap->height))
    return;
  p = bitmap->bits + coord.y * bitmap->rowbytes + coord.x / 8;
  if (value)
    *p |= pixel_mask (coord.x & 7);
  else
    *p &= (0xff ^ pixel_mask (coord.x & 7));
}


Bitmap *bitblt (Bitmap *src_bitmap,
		Rect src_rect,
		Bitmap *dest_bitmap,
		Point dest_upper_left,
		int scan,
		int tfn)
{
  Point src_point, dest_point;
  boolean src_pixel, dest_pixel;

  if (! dest_bitmap)
    {
      if (scan & TRANSPOSE)
	dest_bitmap = create_bitmap (dest_upper_left.x + rect_height (src_rect),
				     dest_upper_left.y + rect_width (src_rect));
      else
	dest_bitmap = create_bitmap (dest_upper_left.x + rect_width (src_rect),
				     dest_upper_left.y + rect_height (src_rect));
      if (! dest_bitmap)
	return (NULL);
    }

  for (src_point.y = src_rect.upper_left.y;
       src_point.y < src_rect.lower_right.y;
       src_point.y++)
    {
      for (src_point.x = src_rect.upper_left.x;
	   src_point.x < src_rect.lower_right.x;
	   src_point.x++)
	{
	  boolean a, b, c;

	  if (scan & TRANSPOSE)
	    {
	      dest_point.x = src_point.y - src_rect.upper_left.y;
	      dest_point.y = src_point.x - src_rect.upper_left.x;

	      if (scan & FLIP_H)
		dest_point.x = (rect_height (src_rect) - 1) - dest_point.x;

	      if (scan & FLIP_V)
		dest_point.y = (rect_width (src_rect) - 1) - dest_point.y;
	    }
	  else
	    {
	      dest_point.x = src_point.x - src_rect.upper_left.x;
	      dest_point.y = src_point.y - src_rect.upper_left.y;

	      if (scan & FLIP_H)
		dest_point.x = (rect_width (src_rect) - 1) - dest_point.x;

	      if (scan & FLIP_V)
		dest_point.y = (rect_height (src_rect) - 1) - dest_point.y;
	    }

	  dest_point.x += dest_upper_left.x;
	  dest_point.y += dest_upper_left.y;

	  a = get_pixel (src_bitmap, src_point);
	  b = get_pixel (dest_bitmap, dest_point);
	  c = (tfn & (1 << (a * 2 + b))) != 0;

	  set_pixel (dest_bitmap, dest_point, c);
	}
    }
  return (dest_bitmap);
}
