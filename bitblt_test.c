#include <stdio.h>

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

  b = create_bitmap (WIDTH, HEIGHT);
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
  printf ("rowbytes: %d\n", b->rowbytes);
  for (p.y = 0; p.y < b->height; p.y++)
    {
      for (p.x = 0; p.x < b->width; p.x++)
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

#if 0
  b2 = create_bitmap (b->height, b->width);
  if (! b2)
    {
      fprintf (stderr, "create_bitmap failed\n");
      exit (2);
    }
#endif

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

  exit (0);
}


