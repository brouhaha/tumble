#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitblt.h"


#define WIDTH 44
#define HEIGHT 9

char test_data [HEIGHT][WIDTH] =
{
  ".....XXXXXXXXXX.......XX............X......X",
  ".....XX.......X.......X.X..........X......X.",
  "XXXXXX.X......XXXXXX..X..X........X......X..",
  ".....X..X.....X.......XX.........X......X...",
  ".....X...X....X.......X.X.......X......X....",
  ".....X....X...X.......X..X.....X......X.....",
  ".....X.....X..X.......XX..... X......X......",
  ".....XXXXXXXXXX.......X.X....X......X.......",
  ".....X.X.X.X.X........X..X..X......X........"
};

Bitmap *setup (void)
{
  Bitmap *b;
  Point p;
  Rect r = {{ 0, 0 }, { WIDTH, HEIGHT }};

  b = create_bitmap (& r);
  if (! b)
    return (NULL);

  for (p.y = 0; p.y < HEIGHT; p.y++)
    for (p.x = 0; p.x < WIDTH; p.x++)
      set_pixel (b, p, test_data [p.y][p.x] == 'X');

  return (b);
}

void print_bitmap (FILE *o, Bitmap *b)
{
  Point p;
  printf ("row_words: %d\n", b->row_words);
  for (p.y = b->rect.min.y; p.y < b->rect.max.y; p.y++)
    {
      for (p.x = b->rect.min.x; p.x < b->rect.max.x; p.x++)
	fputc (".X" [get_pixel (b, p)], o);
      fprintf (o, "\n");
    }
}


int main (int argc, char *argv[])
{
  Bitmap *b;
  Bitmap *b2;
  Rect r;
  Point p;

  b = setup ();
  if (! b)
    {
      fprintf (stderr, "setup failed\n");
      exit (2);
    }

  print_bitmap (stdout, b);
  printf ("\n");

  flip_v (b);

  printf ("flipped vertically:\n");
  print_bitmap (stdout, b);
  printf ("\n");

  flip_h (b);

  printf ("flipped horizontally:\n");
  print_bitmap (stdout, b);
  printf ("\n");

#if 1
  r.min.x = r.min.y = 0;
  r.max.x = b->rect.max.x + 8;
  r.max.y = b->rect.max.y + 8;

  b2 = create_bitmap (& r);

  r.min.x = r.min.y = 0;
  r.max.x = b->rect.max.x;
  r.max.y = b->rect.max.y;

  p.x = -3;
  p.y = -3;

  b2 = bitblt (b, & r,
	       b2, & p,
	       TF_SRC, 0);
  if (! b2)
    {
      fprintf (stderr, "bitblt failed\n");
      exit (2);
    }

  printf ("after bitblt\n");
  print_bitmap (stdout, b2);
#endif

  exit (0);
}


