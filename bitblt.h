typedef struct Point
{
  s32 x;
  s32 y;
} Point;

typedef struct Rect
{
  Point min;
  Point max;
} Rect;

static inline s32 rect_width (Rect *r)
{
  return (r->max.x - r->min.x);
}

static inline s32 rect_height (Rect *r)
{
  return (r->max.y - r->min.y);
}


typedef u32 word_type;
#define BITS_PER_WORD (8 * sizeof (word_type))
#define ALL_ONES (~ 0U)


typedef struct Bitmap
{
  word_type *bits;
  Rect rect;
  u32 row_words;
} Bitmap;


#define TF_SRC 0xc
#define TF_AND 0x8
#define TF_OR  0xe
#define TF_XOR 0x6


Bitmap *create_bitmap (Rect *rect);
void free_bitmap (Bitmap *bitmap);

boolean get_pixel (Bitmap *bitmap, Point coord);
void set_pixel (Bitmap *bitmap, Point coord, boolean value);


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


void reverse_bits (u8 *p, int byte_count);
