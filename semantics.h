typedef struct 
{
  double width;
  double height;
} page_size_t;

typedef struct
{
  int first; 
  int last;
 } range_t;

typedef struct
{
  double left;
  double right;
  double top;
  double bottom;
} crop_t;

typedef struct
{
  boolean has_size;
  page_size_t size;

  boolean has_rotation;
  int rotation;

  boolean has_crop;
  crop_t crop;
} input_modifiers_t;


typedef enum
{
  INPUT_MODIFIER_ALL,
  INPUT_MODIFIER_ODD,
  INPUT_MODIFIER_EVEN,
  INPUT_MODIFIER_TYPE_COUNT  /* must be last */
} input_modifier_type_t;


typedef struct input_context_t
{
  struct input_context_t *parent;
  struct input_context_t *next;

  int image_count;  /* how many pages reference this context,
		      including those from subcontexts */

  char *input_file;

  input_modifiers_t modifiers [INPUT_MODIFIER_TYPE_COUNT];
} input_context_t;


typedef struct input_image_t
{
  struct input_image_t *next;
  input_context_t *input_context;
  range_t range;
} input_image_t;


typedef struct bookmark_t
{
  struct bookmark_t *next;
  char *name;
} bookmark_t;


typedef struct output_context_t
{
  struct output_context_t *parent;
  struct output_context_t *next;

  int page_count;  /* how many pages reference this context,
		      including those from subcontexts */

  char *output_file;
  bookmark_t *first_bookmark;
  bookmark_t *last_bookmark;
  char *page_number_format;
} output_context_t;


typedef struct output_page_t
{
  struct output_page_t *next;
  output_context_t *output_context;
  range_t range;
  bookmark_t *bookmark_list;
} output_page_t;


extern int line;  /* line number in spec file */


boolean parse_spec_file (char *fn);


/* semantic routines for input statements */
void input_push_context (void);
void input_pop_context (void);
void input_set_modifier_context (input_modifier_type_t type);
void input_set_file (char *name);
void input_set_rotation (int rotation);
void input_images (range_t range);

/* semantic routines for output statements */
void output_set_file (char *name);
void output_set_bookmark (char *name);
void output_set_page_number_format (char *format);
void output_pages (range_t range);
