/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * G4 compression
 * $Id: bitblt_g4.c,v 1.13 2003/03/12 19:30:44 eric Exp $
 * Copyright 2003 Eric Smith <eric@brouhaha.com>
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


#define G4_DEBUG 0


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "bitblt.h"
#include "bitblt_tables.h"
#include "pdf_util.h"


#include "g4_tables.h"


#define BIT_BUF_SIZE 4096

struct bit_buffer
{
  FILE *f;
  uint32_t byte_idx;  /* index to next byte position in data buffer */
  uint32_t bit_idx;   /* one greater than the next bit position in data buffer,
			 8 = MSB, 1 = LSB */
  uint8_t data [BIT_BUF_SIZE];
};


static void init_bit_buffer (struct bit_buffer *buf)
{
  buf->byte_idx = 0;
  buf->bit_idx = 8;
  memset (& buf->data [0], 0, BIT_BUF_SIZE);
}


static void flush_bits (struct bit_buffer *buf)
{
  size_t s;
  if (buf->bit_idx != 8)
    {
      buf->byte_idx++;
      buf->bit_idx = 8;
    }
  s = fwrite (& buf->data [0], 1, buf->byte_idx, buf->f);
  /* $$$ should check result */
  init_bit_buffer (buf);
}


static void advance_byte (struct bit_buffer *buf)
{
  buf->byte_idx++;
  buf->bit_idx = 8;
  if (buf->byte_idx == BIT_BUF_SIZE)
    flush_bits (buf);
}


static void write_bits (struct bit_buffer *buf,
			uint32_t count,
			uint32_t bits)
{
  while (count > buf->bit_idx)
    {
      buf->data [buf->byte_idx] |= bits >> (count - buf->bit_idx);
      count -= buf->bit_idx;
      advance_byte (buf);
    }

  bits &= ((1 << count) - 1);
  buf->data [buf->byte_idx] |= bits << (buf->bit_idx - count);
  buf->bit_idx -= count;
  if (buf->bit_idx == 0)
    advance_byte (buf);
}


static void g4_encode_horizontal_run (struct bit_buffer *buf,
				      bool black,
				      uint32_t run_length)
{
  uint32_t i;

  while (run_length >= 2560)
    {
      write_bits (buf, 12, 0x01f);
      run_length -= 2560;
    }

  if (run_length >= 1792)
    {
      i = (run_length - 1792) >> 6;
      write_bits (buf,
		  g4_long_makeup_code [i].count,
		  g4_long_makeup_code [i].bits);
      run_length -= (1792 + (i << 6));
    }
  else if (run_length >= 64)
    {
      i = (run_length >> 6) - 1;
      write_bits (buf,
		  g4_makeup_code [black] [i].count,
		  g4_makeup_code [black] [i].bits);
      run_length -= (i + 1) << 6;
    }

  write_bits (buf,
	      g4_h_code [black] [run_length].count,
	      g4_h_code [black] [run_length].bits);
}


static inline int g4_get_pixel (uint8_t *buf, uint32_t x)
{
  return ((buf [x >> 3] >> (x & 7)) & 1);
}


#define not_aligned(p) (((word_t) p) & (sizeof (word_t) - 1))


static uint32_t g4_find_pixel (uint8_t *buf,
			       uint32_t pos,
			       uint32_t width,
			       bool color)
{
  uint8_t *p = buf + pos / 8;
  int bit = pos & 7;
  uint8_t *max_p = buf + (width - 1) / 8;
  uint8_t d;

  /* check first byte (may be partial) */
  d = *p;
  if (! color)
    d = ~d;
  bit += rle_tab [bit][d];
  if (bit < 8)
    goto done;
  p++;

  /* check individual bytes until we hit word alignment */
  while ((p <= max_p) && not_aligned (p))
    {
      d = *p;
      if (! color)
	d = ~d;
      if (d != 0)
	goto found;
      p++;
    }

  /* check aligned words in middle */
  while ((p <= (max_p - sizeof (word_t))))
    {
      word_t w = *((word_t *) p);
      if (! color)
	w = ~w;
      if (w != 0)
	break;
      p += sizeof (word_t);
    }

  /* check trailing bytes */
  while (p <= max_p)
    {
      d = *p;
      if (! color)
	d = ~d;
      if (d)
	goto found;
      p++;
    }

  goto not_found;

 found:
  bit = rle_tab [0][d];

 done:
  pos = ((p - buf) << 3) + bit;
  if (pos < width)
    return (pos);

 not_found:
  return (width);
}


static void g4_encode_row (struct bit_buffer *buf,
			   uint32_t width,
			   uint8_t *ref,
			   uint8_t *row)
{
  uint32_t a0, a1, a2;
  uint32_t b1, b2;
  bool a0_c;

  a0 = 0;
  a0_c = 0;

  a1 = g4_find_pixel (row, 0, width, 1);

  b1 = g4_find_pixel (ref, 0, width, 1);

#if (G4_DEBUG & 1)
  fprintf (stderr, "start of row\n");
  if ((a1 != width) || (b1 != width))
    {
      fprintf (stderr, "a1 = %u, b1 = %u\n", a1, b1);
    }
#endif
  
  while (a0 < width)
    {
      b2 = g4_find_pixel (ref, b1 + 1, width, ! g4_get_pixel (ref, b1));

      if (b2 < a1)
	{
	  /* pass mode - 0001 */
	  write_bits (buf, 4, 0x1);
	  a0 = b2;
#if (G4_DEBUG & 1)
	  fprintf (stderr, "pass\n");
#endif
	}
      else if (abs (a1 - b1) <= 3)
	{
	  /* vertical mode */
	  write_bits (buf,
		      g4_vert_code [3 + a1 - b1].count,
		      g4_vert_code [3 + a1 - b1].bits);
	  a0 = a1;
#if (G4_DEBUG & 1)
	  fprintf (stderr, "vertical %d\n", a1 - b1);
#endif
	}
      else
	{
	  /* horizontal mode - 001 */
	  a2 = g4_find_pixel (row, a1 + 1, width, a0_c);
	  write_bits (buf, 3, 0x1);
	  g4_encode_horizontal_run (buf,   a0_c, a1 - a0);
	  g4_encode_horizontal_run (buf, ! a0_c, a2 - a1);
#if (G4_DEBUG & 1)
	  fprintf (stderr, "horizontal %d %s, %d %s\n",
		   a1 - a0, a0_c ? "black" : "white",
		   a2 - a1, a0_c ? "white" : "black");
#endif
	  a0 = a2;
	}

      if (a0 >= width)
	break;;

      a0_c = g4_get_pixel (row, a0);

      a1 = g4_find_pixel (row, a0 + 1, width, ! a0_c);
      b1 = g4_find_pixel (ref, a0 + 1, width, ! g4_get_pixel (ref, a0));
      if (g4_get_pixel (ref, b1) == a0_c)
	b1 = g4_find_pixel (ref, b1 + 1, width, ! a0_c);
#if (G4_DEBUG & 1)
      fprintf (stderr, "a1 = %u, b1 = %u\n", a1, b1);
#endif
    }
}


void bitblt_write_g4 (Bitmap *bitmap, FILE *f)
{
  uint32_t width = bitmap->rect.max.x - bitmap->rect.min.x;
  uint32_t row;
  struct bit_buffer bb;

  word_t *temp_buffer;

  word_t *cur_line;
  word_t *ref_line;  /* reference (previous) row */

  temp_buffer = pdf_calloc ((width + BITS_PER_WORD - 1) / BITS_PER_WORD,
			    sizeof (word_t));

  cur_line = bitmap->bits;
  ref_line = temp_buffer;

  init_bit_buffer (& bb);

  bb.f = f;

  for (row = bitmap->rect.min.y;
       row < bitmap->rect.max.y;
       row++)
    {
      g4_encode_row (& bb,
		     width,
		     (uint8_t *) ref_line,
		     (uint8_t *) cur_line);
      ref_line = cur_line;
      cur_line += bitmap->row_words;
    }

  
  /* write EOFB code */
  write_bits (& bb, 24, 0x001001);

  flush_bits (& bb);

  free (temp_buffer);
}


