/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * G4 compression
 * $Id: bitblt_g4.c,v 1.9 2003/03/10 01:49:50 eric Exp $
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


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "bitblt.h"


#include "g4_tables.h"


#define BIT_BUF_SIZE 4096

struct bit_buffer
{
  FILE *f;
  uint32_t byte_idx;  /* index to next byte position in data buffer */
  uint32_t bit_idx;   /* index to next bit position in data buffer,
			 0 = MSB, 7 = LSB */
  uint8_t data [BIT_BUF_SIZE];
};


static void flush_bits (struct bit_buffer *buf)
{
  size_t s;
  if (buf->bit_idx)
    {
      /* zero remaining bits in last byte */
      buf->data [buf->byte_idx] &= ~ ((1 << (8 - buf->bit_idx)) - 1);
      buf->byte_idx++;
      buf->bit_idx = 0;
    }
  s = fwrite (& buf->data [0], 1, buf->byte_idx, buf->f);
  /* $$$ should check result */
  buf->byte_idx = 0;
}


static void advance_byte (struct bit_buffer *buf)
{
  buf->byte_idx++;
  buf->bit_idx = 0;
  if (buf->byte_idx == BIT_BUF_SIZE)
    flush_bits (buf);
}


static void write_bits (struct bit_buffer *buf,
			uint32_t count,
			uint32_t bits)
{
  uint32_t b2;  /* how many bits will fit in byte in data buffer */
  uint32_t c2;  /* how many bits to transfer on this iteration */
  uint32_t d2;  /* bits to transfer on this iteration */

  while (count)
    {
      b2 = 8 - buf->bit_idx;
      if (b2 >= count)
	c2 = count;
      else
	c2 = b2;
      d2 = bits >> (count - c2);
      buf->data [buf->byte_idx] |= (d2 << (b2 + c2));
      buf->bit_idx += c2;
      if (buf->bit_idx > 7)
	advance_byte (buf);
      count -= c2;
    }
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


static uint32_t find_transition (uint8_t *data,
				 uint32_t pos,
				 uint32_t width)
{
  if (! data)
    return (width);
  return (0);  /* $$$ */
}


static void g4_encode_row (struct bit_buffer *buf,
			   uint32_t width,
			   uint8_t *ref,
			   uint8_t *row)
{
  int a0, a1, a2;
  int b1, b2;

  a0 = -1;
  
  while (a0 < width)
    {
      /* find a1, a2 */
      a1 = find_transition (row, a0, width);
      a2 = find_transition (row, a1, width);

      /* find b1, b2 */
      b1 = find_transition (ref, a0, width);
      if (0) /* $$$ b1 color = a0 color */
	b1 = find_transition (ref, b1, width);
      b2 = find_transition (ref, b2, width);

      if (b2 < a1)
	{
	  /* pass mode - 0001 */
	  write_bits (buf, 4, 0x1);
	  a0 = b2;
	}
      else if (abs (a1 - b1) <= 3)
	{
	  /* vertical mode */
	  write_bits (buf,
		      g4_vert_code [3 + a1 - b1].count,
		      g4_vert_code [3 + a1 - b1].bits);
	  a0 = a1;
	}
      else
	{
	  /* horizontal mode - 001 */
	  write_bits (buf, 3, 0x1);
	  g4_encode_horizontal_run (buf, 0 /* $$$ color (a0) */, a1 - a0);
	  g4_encode_horizontal_run (buf, 1 /* $$$ color (a1) */, a2 - a1);
	  a0 = a2;
	}
    }
}


void bitblt_write_g4 (Bitmap *bitmap, FILE *f)
{
  uint32_t row;
  struct bit_buffer bb;

  word_type *ref_line = NULL;  /* reference (previous) row */
  word_type *line = bitmap->bits;

  memset (& bb, 0, sizeof (bb));

  bb.f = f;

  for (row = bitmap->rect.min.y;
       row < bitmap->rect.max.y;
       row++)
    {
      g4_encode_row (& bb,
		     (bitmap->rect.max.x - bitmap->rect.min.x) + 1,
		     (uint8_t *) ref_line,
		     (uint8_t *) line);
      ref_line = line;
      line += bitmap->row_words;
    }

  
  /* write EOFB code */
  write_bits (& bb, 24, 0x001001);

  flush_bits (& bb);
}


