/*
 * t2p: Create a PDF file from the contents of one or more TIFF
 *      bilevel image files.  The images in the resulting PDF file
 *      will be compressed using ITU-T T.6 (G4) fax encoding.
 *
 * G4 table generator
 * $Id: g4_table_gen.c,v 1.2 2003/03/05 12:44:33 eric Exp $
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


void emit_code (int indent, char *code, int last, bool comment, int cval)
{
  int i;
  int count = 0;
  uint32_t val = 0;

  printf ("%*s{ ", indent, "");

  printf ("%d, ", strlen (code));

  for (i = 0; i < strlen (code); i++)
    switch (code [i])
      {
      case '0': val = (val << 1);     count++; break;
      case '1': val = (val << 1) + 1; count++; break;
      case ' ': break;
      default:
	fprintf (stderr, "internal error\n");
	exit (2);
      }

  printf ("0x%0*x", (count + 3)/4, val);

  printf (" }");
  if (! last)
    printf (",");
  if (comment)
    printf ("  /* %d */", cval);
  printf ("\n");
}


char *long_makeup_code [12] =
  {
    /* 1792 */ "00000001000",
    /* 1856 */ "00000001100",
    /* 1920 */ "00000001101",
    /* 1984 */ "000000010010",
    /* 2048 */ "000000010011",
    /* 2112 */ "000000010100",
    /* 2176 */ "000000010101",
    /* 2240 */ "000000010110",
    /* 2304 */ "000000010111",
    /* 2368 */ "000000011100",
    /* 2432 */ "000000011101",
    /* 2496 */ "000000011110"
    /* 2560    "000000011111"  hard-coded, doesn't need to be in table */
  };


void print_long_makeup_code (void)
{
  int i;

  printf ("static g4_bits g4_long_makeup_code [12] =\n");
  printf ("  {\n");
  for (i = 0; i < 12; i++)
    emit_code (4, long_makeup_code [i], i == 11, 1, i * 64 + 1792);
  printf ("  };\n");
}


char *makeup_code [64][2] =
  {
    { /*   64 */ "11011",     "0000001111" },
    { /*  128 */ "10010",     "000011001000" },
    { /*  192 */ "010111",    "000011001001" },
    { /*  256 */ "0110111",   "000001011011" },
    { /*  320 */ "00110110",  "000000110011" },
    { /*  384 */ "00110111",  "000000110100" },
    { /*  448 */ "01100100",  "000000110101" },
    { /*  512 */ "01100101",  "0000001101100" },
    { /*  576 */ "01101000",  "0000001101101" },
    { /*  640 */ "01100111",  "0000001001010" },
    { /*  704 */ "011001100", "0000001001011" },
    { /*  768 */ "011001101", "0000001001100" },
    { /*  832 */ "011010010", "0000001001101" },
    { /*  896 */ "011010011", "0000001110010" },
    { /*  960 */ "011010100", "0000001110011" },
    { /* 1024 */ "011010101", "0000001110100" },
    { /* 1088 */ "011010110", "0000001110101" },
    { /* 1152 */ "011010111", "0000001110110" },
    { /* 1216 */ "011011000", "0000001110111" },
    { /* 1280 */ "011011001", "0000001010010" },
    { /* 1344 */ "011011010", "0000001010011" },
    { /* 1408 */ "011011011", "0000001010100" },
    { /* 1472 */ "010011000", "0000001010101" },
    { /* 1536 */ "010011001", "0000001011010" },
    { /* 1600 */ "010011010", "0000001011011" },
    { /* 1664 */ "011000",    "0000001100100" },
    { /* 1728 */ "010011011", "0000001100101" }
  };


void print_makeup_code (void)
{
  int i;

  printf ("static g4_bits g4_makeup_code [2] [27] =\n");
  printf ("  {\n");
  printf ("    {\n");
  printf ("      /* white */\n");
  for (i = 0; i <= 26; i++)
    emit_code (6, makeup_code [i][0], i == 26, 1, (i + 1) * 64);
  printf ("    },\n");
  printf ("    {\n");
  printf ("      /* black */\n");
  for (i = 0; i <= 26; i++)
    emit_code (6, makeup_code [i][1], i == 26, 1, (i + 1) * 64);
  printf ("    }\n");
  printf ("  };\n");
}


char *h_code [64][2] =
  {
    { /*  0 */ "00110101", "0000110111" },
    { /*  1 */ "000111",   "010" },
    { /*  2 */ "0111",     "11" },
    { /*  3 */ "1000",     "10" },
    { /*  4 */ "1011",     "011" },
    { /*  5 */ "1100",     "0011" },
    { /*  6 */ "1110",     "0010" },
    { /*  7 */ "1111",     "00011" },
    { /*  8 */ "10011",    "000101" },
    { /*  9 */ "10100",    "000100" },
    { /* 10 */ "00111",    "0000100" },
    { /* 11 */ "01000",    "0000101" },
    { /* 12 */ "001000",   "0000111" },
    { /* 13 */ "000011",   "00000100" },
    { /* 14 */ "110100",   "00000111" },
    { /* 15 */ "110101",   "000011000" },
    { /* 16 */ "101010",   "0000010111" },
    { /* 17 */ "101011",   "0000011000" },
    { /* 18 */ "0100111",  "0000001000" },
    { /* 19 */ "0001100",  "00001100111" },
    { /* 20 */ "0001000",  "00001101000" },
    { /* 21 */ "0010111",  "00001101100" },
    { /* 22 */ "0000011",  "00000110111" },
    { /* 23 */ "0000100",  "00000101000" },
    { /* 24 */ "0101000",  "00000010111" },
    { /* 25 */ "0101011",  "00000011000" },
    { /* 26 */ "0010011",  "000011001010" },
    { /* 27 */ "0100100",  "000011001011" },
    { /* 28 */ "0011000",  "000011001100" },
    { /* 29 */ "00000010", "000011001101" },
    { /* 30 */ "00000011", "000001101000" },
    { /* 31 */ "00011010", "000001101001" },
    { /* 32 */ "00011011", "000001101010" },
    { /* 33 */ "00010010", "000001101011" },
    { /* 34 */ "00010011", "000011010010" },
    { /* 35 */ "00010100", "000011010011" },
    { /* 36 */ "00010101", "000011010100" },
    { /* 37 */ "00010110", "000011010101" },
    { /* 38 */ "00010111", "000011010110" },
    { /* 39 */ "00101000", "000011010111" },
    { /* 40 */ "00101001", "000001101100" },
    { /* 41 */ "00101010", "000001101101" },
    { /* 42 */ "00101011", "000011011010" },
    { /* 43 */ "00101100", "000011011011" },
    { /* 44 */ "00101101", "000001010100" },
    { /* 45 */ "00000100", "000001010101" },
    { /* 46 */ "00000101", "000001010110" },
    { /* 47 */ "00001010", "000001010111" },
    { /* 48 */ "00001011", "000001100100" },
    { /* 49 */ "01010010", "000001100101" },
    { /* 50 */ "01010011", "000001010010" },
    { /* 51 */ "01010100", "000001010011" },
    { /* 52 */ "01010101", "000000100100" },
    { /* 53 */ "00100100", "000000110111" },
    { /* 54 */ "00100101", "000000111000" },
    { /* 55 */ "01011000", "000000100111" },
    { /* 56 */ "01011001", "000000101000" },
    { /* 57 */ "01011010", "000001011000" },
    { /* 58 */ "01011011", "000001011001" },
    { /* 59 */ "01001010", "000000101011" },
    { /* 60 */ "01001011", "000000101100" },
    { /* 61 */ "00110010", "000001011010" },
    { /* 62 */ "00110011", "000001100110" },
    { /* 63 */ "00110100", "000001100111" }
  };


void print_h_code (void)
{
  int i;

  printf ("static g4_bits g4_h_code [2] [64] =\n");
  printf ("  {\n");
  printf ("    {\n");
  printf ("      /* white */\n");
  for (i = 0; i <= 63; i++)
    emit_code (6, h_code [i][0], i == 63, 1, i);
  printf ("    },\n");
  printf ("    {\n");
  printf ("      /* black */\n");
  for (i = 0; i <= 63; i++)
    emit_code (6, h_code [i][1], i == 63, 1, i);
  printf ("    }\n");
  printf ("  };\n");
}


char *v_code [7] =
  {
    /* -3 */ "0000010",
    /* -2 */ "000010",
    /* -1 */ "010",
    /*  0 */ "1",
    /*  1 */ "011",
    /*  2 */ "000011",
    /*  3 */ "0000011"
  };


void print_v_code (void)
{
  int i;

  printf ("static g4_bits g4_vert_code [7] =\n");
  printf ("  {\n");
  for (i = 0; i <= 6; i++)
    emit_code (4, v_code [i], i == 6, 1, i - 3);
  printf ("  };\n");
}


int main (int argc, char *argv [])
{
  printf ("/* This file is automatically generated; do not edit */\n");
  printf ("\n");
  printf ("typedef struct\n");
  printf ("{\n");
  printf ("  uint32_t count;\n");
  printf ("  uint32_t bits;\n");
  printf ("} g4_bits;\n");
  printf ("\n");

  print_long_makeup_code ();
  printf ("\n");

  print_makeup_code ();
  printf ("\n");

  print_h_code ();
  printf ("\n");

  print_v_code ();

  exit (0);
}
