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

typedef enum
{
  INPUT_CONTEXT_ALL,
  INPUT_CONTEXT_ODD,
  INPUT_CONTEXT_EVEN
} input_context_type_t;


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
  struct input_context_t *parent_input_context;

  int page_count;  /* how many pages reference this context,
		      including those from subcontexts */

  input_modifiers_t modifiers [INPUT_MODIFIER_TYPE_COUNT];
} input_context_t;


extern int line;  /* line number in spec file */


extern int input_page_count;   /* total input pages in spec */
extern int output_page_count;  /* total output pages in spec */


boolean parse_spec_file (char *fn);


/* semantic routines for input statements */
void input_push_context (input_context_type_t type);
void input_pop_context (void);
void input_set_file (char *name);
void input_images (int first, int last);

/* semantic routines for output statements */
void output_set_file (char *name);
void output_pages (int first, int last);
