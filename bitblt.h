typedef unsigned char u8;
typedef unsigned int u32;
typedef int boolean;

typedef struct Point
{
  u32 x;
  u32 y;
} Point;

typedef struct Rect
{
  Point upper_left;
  Point lower_right;
} Rect;

typedef struct Bitmap
{
  u8 *bits;
  u32 width;
  u32 height;
  u32 rowbytes;
} Bitmap;


#define TF_SRC 0xc
#define TF_AND 0x8
#define TF_OR  0xe
#define TF_XOR 0x6


#define FLIP_H    0x1
#define FLIP_V    0x2
#define TRANSPOSE 0x4

#define ROT_0     0x0
#define ROT_90    (TRANSPOSE + FLIP_H)
#define ROT_180   (FLIP_H + FLIP_V)
#define ROT_270   (TRANSPOSE + FLIP_V)


Bitmap *create_bitmap (u32 width, u32 height);
void free_bitmap (Bitmap *bitmap);
boolean get_pixel (Bitmap *bitmap, Point coord);
void set_pixel (Bitmap *bitmap, Point coord, boolean value);

Bitmap *bitblt (Bitmap *src_bitmap,
		Rect src_rect,
		Bitmap *dest_bitmap,
		Point dest_upper_left,
		int scan,
		int tfn);
