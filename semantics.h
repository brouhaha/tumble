typedef enum
{
  INPUT_CONTEXT_ALL,
  INPUT_CONTEXT_ODD,
  INPUT_CONTEXT_EVEN
} input_context_type_t;


extern int line;  /* line number in spec file */


extern int input_count;   /* total input pages in spec */
extern int output_count;  /* total output pages in spec */


boolean parse_spec_file (char *fn);


/* semantic routines for input statements */
void input_push_context (input_context_type_t type);
void input_pop_context (void);
void input_set_file (char *name);
void input_images (int first, int last);

/* semantic routines for output statements */
void output_set_file (char *name);
void output_pages (int first, int last);
