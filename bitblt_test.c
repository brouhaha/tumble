#include <stdio.h>
#include <stdlib.h>

#include "type.h"
#include "bitblt.h"


#define WIDTH 20
#define HEIGHT 9

char test_data [HEIGHT][WIDTH] =
{
  ".....XXXXXXXXXX.....",
  ".....XX.......X.....",
  "XXXXXX.X......XXXXXX",
  ".....X..X.....X.....",
  ".....X...X....X.....",
  ".....X....X...X.....",
  ".....X.....X..X.....",
  ".....XXXXXXXXXX.....",
  ".....X.X.X.X.X......"
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

  print_bitmap (stdout, b);
  printf ("\n");

  flip_h (b);

  print_bitmap (stdout, b);
  printf ("\n");

#if 0
  r.upper_left.x = r.upper_left.y = 0;
  r.lower_right.x = b->width;
  r.lower_right.y = b->height;
  p.x = p.y = 0;

  b2 = bitblt (b, r,
	       NULL, p,
	       ROT_90,
	       TF_SRC);
  if (! b2)
    {
      fprintf (stderr, "bitblt failed\n");
      exit (2);
    }

  print_bitmap (stdout, b2);
#endif

  exit (0);
}


