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
  char *prefix;
  char style;
} page_label_t;


typedef enum
{
  INPUT_MODIFIER_ALL,
  INPUT_MODIFIER_ODD,
  INPUT_MODIFIER_EVEN,
  INPUT_MODIFIER_TYPE_COUNT  /* must be last */
} input_modifier_type_t;


typedef struct bookmark_t
{
  struct bookmark_t *next;
  int level;  /* 1 is outermost */
  char *name;
} bookmark_t;


extern int line;  /* line number in spec file */
extern int bookmark_level;


/* semantic routines for input statements */
void input_push_context (void);
void input_pop_context (void);
void input_set_modifier_context (input_modifier_type_t type);
void input_set_file (char *name);
void input_set_rotation (int rotation);
void input_set_page_size (page_size_t size);
void input_images (range_t range);

/* semantic routines for output statements */
void output_push_context (void);
void output_pop_context (void);

void output_set_file (char *name);
void output_set_author (char *author);
void output_set_creator (char *creator);
void output_set_title (char *title);
void output_set_subject (char *subject);
void output_set_keywords (char *keywords);

void output_set_bookmark (char *name);
void output_set_page_label (page_label_t label);
void output_pages (range_t range);


/* functions to be called from main program: */
boolean parse_spec_file (char *fn);
boolean process_specs (void);
