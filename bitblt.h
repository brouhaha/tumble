/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * bitblt routines
 * $Id: bitblt.h,v 1.12 2003/02/23 09:40:41 eric Exp $
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


typedef struct Point
{
  int32_t x;
  int32_t y;
} Point;

typedef struct Rect
{
  Point min;
  Point max;
} Rect;

static inline int32_t rect_width (Rect *r)
{
  return (r->max.x - r->min.x);
}

static inline int32_t rect_height (Rect *r)
{
  return (r->max.y - r->min.y);
}


typedef uint32_t word_type;
#define BITS_PER_WORD (8 * sizeof (word_type))
#define ALL_ONES (~ 0U)


typedef struct Bitmap
{
  word_type *bits;
  Rect rect;
  uint32_t row_words;
} Bitmap;


#define TF_SRC 0xc
#define TF_AND 0x8
#define TF_OR  0xe
#define TF_XOR 0x6


void bitblt_init (void);


Bitmap *create_bitmap (Rect *rect);
void free_bitmap (Bitmap *bitmap);

bool get_pixel (Bitmap *bitmap, Point coord);
void set_pixel (Bitmap *bitmap, Point coord, bool value);


Bitmap *bitblt (Bitmap *src_bitmap,
		Rect   *src_rect,
		Bitmap *dest_bitmap,
		Point  *dest_min,
		int tfn,
		int background);


/* in-place transformations */
void flip_h (Bitmap *src);
void flip_v (Bitmap *src);

void rot_180 (Bitmap *src);  /* combination of flip_h and flip_v */

/* "in-place" transformations - will allocate new memory and free old */
void transpose (Bitmap *src);

void rot_90 (Bitmap *src);   /* transpose + flip_h */
void rot_270 (Bitmap *src);  /* transpose + flip_v */


void reverse_bits (uint8_t *p, int byte_count);


/*
 * get_row_run_lengths counts the runs of 0 and 1 bits in row
 * y of a bitmap, from min_x through max_x inclusive.  The run lengths
 * will be stored in the run_length array.  The first entry will be
 * the length of a zero run (which length may be zero, if the first
 * bit is a one).  The next entry will the be the length of a run of
 * ones, and they will alternate from there, with even entries representing
 * runs of zeros, and odd entries representing runs of ones.
 *
 * max_runs should be set to the maximum number of run lengths that
 * can be stored in the run_length array.
 *
 * Returns the actual number of runs counted, or -max_runs if there
 * was not enough room in the array.
 */

typedef struct
{
  bool value;
  int32_t left;
  uint32_t width;
} run_t;

int32_t get_row_run_lengths (Bitmap *src,
			     int32_t y,
			     int32_t min_x, int32_t max_x,
			     int32_t max_runs,
			     run_t *runs);
