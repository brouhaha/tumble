#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "type.h"
#include "semantics.h"
#include "parser.tab.h"


typedef struct
{
  boolean has_size;
  page_size_t size;

  boolean has_rotation;
  int rotation;

  boolean has_crop;
  crop_t crop;
} input_modifiers_t;


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


#define SEMANTIC_DEBUG
#ifdef SEMANTIC_DEBUG
#define SDBG(x) printf x
#else
#define SDBG(x)
#endif


FILE *yyin;
int line;  /* line number in spec file */

int bookmark_level;

input_context_t *first_input_context;
input_context_t *last_input_context;

input_modifier_type_t current_modifier_context;

input_image_t *first_input_image;
input_image_t *last_input_image;

output_context_t *first_output_context;
output_context_t *last_output_context;

output_page_t *first_output_page;
output_page_t *last_output_page;


void input_push_context (void)
{
  input_context_t *new_input_context;

  new_input_context = malloc (sizeof (input_context_t));
  if (! new_input_context)
    {
      fprintf (stderr, "failed to malloc an input context\n");
      return;
    }

  if (last_input_context)
    {
      memcpy (new_input_context, last_input_context, sizeof (input_context_t));
      new_input_context->image_count = 0;
    }
  else
    {
      memset (new_input_context, 0, sizeof (input_context_t));
      first_input_context = new_input_context;
    }

  new_input_context->parent = last_input_context;
  last_input_context = new_input_context;
};

void input_pop_context (void)
{
  if (! last_input_context)
    {
      fprintf (stderr, "failed to pop an input context\n");
      return;
    }

  last_input_context = last_input_context->parent;
};

void input_set_modifier_context (input_modifier_type_t type)
{
  current_modifier_context = type;
#ifdef SEMANTIC_DEBUG
  SDBG(("modifier type "));
  switch (type)
    {
    case INPUT_MODIFIER_ALL: SDBG(("all")); break;
    case INPUT_MODIFIER_ODD: SDBG(("odd")); break;
    case INPUT_MODIFIER_EVEN: SDBG(("even")); break;
    default: SDBG(("unknown %d", type));
    }
  SDBG(("\n"));
#endif /* SEMANTIC_DEBUG */
}

static void input_clone (void)
{
  input_context_t *new_input_context;

  if (! last_input_context->image_count)
    return;

  new_input_context = malloc (sizeof (input_context_t));
  if (! new_input_context)
    {
      fprintf (stderr, "failed to malloc an input context\n");
      return;
    }

  memcpy (new_input_context, last_input_context, sizeof (input_context_t));
  new_input_context->image_count = 0;
  last_input_context->next = new_input_context;
}

void input_set_file (char *name)
{
  input_clone ();
  last_input_context->input_file = name;
};

void input_set_rotation (int rotation)
{
  last_input_context->modifiers [current_modifier_context].has_rotation = 1;
  last_input_context->modifiers [current_modifier_context].rotation = rotation;
  SDBG(("rotation %d\n", rotation));
}

static void increment_input_image_count (int count)
{
  input_context_t *context;

  for (context = last_input_context; context; context = context->parent)
    context->image_count += count;
}

void input_images (range_t range)
{
  input_image_t *new_image;
  int count = ((range.last - range.first) + 1);

#ifdef SEMANTIC_DEBUG
  if (range.first == range.last)
    SDBG(("image %d\n", range.first));
  else
    SDBG(("images %d..%d\n", range.first, range.last));
#endif /* SEMANTIC_DEBUG */

  new_image = calloc (1, sizeof (input_image_t));
  if (! new_image)
    {
      fprintf (stderr, "failed to malloc an input image struct\n");
      return;
    }
  if (first_input_image)
    {
      last_input_image->next = new_image;
      last_input_image = new_image;
    }
  else
    {
      first_input_image = last_input_image = new_image;
    }
  new_image->range = range;
  new_image->input_context = last_input_context;
  increment_input_image_count (count);
}


void output_push_context (void)
{
  output_context_t *new_output_context;

  new_output_context = malloc (sizeof (output_context_t));
  if (! new_output_context)
    {
      fprintf (stderr, "failed to malloc an output context\n");
      return;
    }

  if (last_output_context)
    {
      memcpy (new_output_context, last_output_context, sizeof (output_context_t));
      new_output_context->page_count = 0;
      new_output_context->first_bookmark = NULL;
      new_output_context->last_bookmark = NULL;
    }
  else
    {
      memset (new_output_context, 0, sizeof (output_context_t));
      first_output_context = new_output_context;
    }

  new_output_context->parent = last_output_context;
  last_output_context = new_output_context;
};

void output_pop_context (void)
{
  if (! last_output_context)
    {
      fprintf (stderr, "failed to pop an output context\n");
      return;
    }

  last_output_context = last_output_context->parent;
};

static void output_clone (void)
{
  output_context_t *new_output_context;

  if (! last_output_context->page_count)
    return;

  new_output_context = malloc (sizeof (output_context_t));
  if (! new_output_context)
    {
      fprintf (stderr, "failed to malloc an output context\n");
      return;
    }

  memcpy (new_output_context, last_output_context, sizeof (output_context_t));
  new_output_context->page_count = 0;
  last_output_context->next = new_output_context;
}

void output_set_file (char *name)
{
  output_clone ();
  last_output_context->output_file = name;
};

void output_set_bookmark (char *name)
{
  bookmark_t *new_bookmark;

  /* As the language is defined (parser.y), a bookmark can only appear
     at the beginning of a context! */
  if (last_output_context->page_count)
    {
      fprintf (stderr, "internal error, bookmark not at beginning of context\n");
      exit (2);
    }

  new_bookmark = calloc (1, sizeof (bookmark_t));
  if (! new_bookmark)
    {
      fprintf (stderr, "failed to calloc a bookmark\n");
      return;
    }

  new_bookmark->level = bookmark_level;
  new_bookmark->name = name;
  if (last_output_context->first_bookmark)
    last_output_context->last_bookmark->next = new_bookmark;
  else
    last_output_context->first_bookmark = new_bookmark;
  last_output_context->last_bookmark = new_bookmark;
}

void output_set_page_number_format (char *format)
{
  output_clone ();
  last_output_context->page_number_format = format;
}

static void increment_output_page_count (int count)
{
  output_context_t *context;

  for (context = last_output_context; context; context = context->parent)
    context->page_count += count;
}


void output_pages (range_t range)
{
  output_page_t *new_page;
  output_context_t *context;
  int count = ((range.last - range.first) + 1);

#ifdef SEMANTIC_DEBUG
  if (range.first == range.last)
    SDBG(("page %d\n", range.first));
  else
    SDBG(("pages %d..%d\n", range.first, range.last));
#endif /* SEMANTIC_DEBUG */

  new_page = calloc (1, sizeof (output_page_t));
  if (! new_page)
    {
      fprintf (stderr, "failed to malloc an output page struct\n");
      return;
    }
  if (first_output_page)
    {
      last_output_page->next = new_page;
      last_output_page = new_page;
    }
  else
    {
      first_output_page = last_output_page = new_page;
    }
  new_page->range = range;
  new_page->output_context = last_output_context;

  /* transfer bookmarks from context(s) to page */
  for (context = last_output_context; context; context = context->parent)
    if (context->first_bookmark)
      {
	context->last_bookmark->next = new_page->bookmark_list;
	new_page->bookmark_list = context->first_bookmark;
	context->first_bookmark = NULL;
	context->last_bookmark = NULL;
      }

  increment_output_page_count (count);
}


void yyerror (char *s)
{
  fprintf (stderr, "%d: %s\n", line, s);
}


static char *get_input_file (input_context_t *context)
{
  for (; context; context = context->parent)
    if (context->input_file)
      return (context->input_file);
  fprintf (stderr, "no input file name found\n");
  exit (2);
}

static int get_input_rotation (input_context_t *context, input_modifier_type_t type)
{
  for (; context; context = context->parent)
    {
      if (context->modifiers [type].has_rotation)
	return (context->modifiers [type].rotation);
      if (context->modifiers [INPUT_MODIFIER_ALL].has_rotation)
	return (context->modifiers [INPUT_MODIFIER_ALL].rotation);
    }
  return (0);  /* default */
}

static char *get_output_file (output_context_t *context)
{
  for (; context; context = context->parent)
    if (context->output_file)
      return (context->output_file);
  fprintf (stderr, "no output file name found\n");
  exit (2);
}

static char *get_output_page_number_format (output_context_t *context)
{
  for (; context; context = context->parent)
    if (context->page_number_format)
      return (context->page_number_format);
  return (NULL);  /* default */
}


#ifdef SEMANTIC_DEBUG
void dump_input_tree (void)
{
  input_image_t *image;
  int i;

  printf ("input images:\n");
  for (image = first_input_image; image; image = image->next)
    for (i = image->range.first; i <= image->range.last; i++)
      {
	input_modifier_type_t parity = (i % 2) ? INPUT_MODIFIER_ODD : INPUT_MODIFIER_EVEN;
	printf ("file '%s' image %d, rotation %d\n",
	        get_input_file (image->input_context),
		i, 
		get_input_rotation (image->input_context, parity));
      }
}

void dump_output_tree (void)
{
  int i;
  output_page_t *page;
  bookmark_t *bookmark;

  printf ("output pages:\n");
  for (page = first_output_page; page; page = page->next)
    {
      if (page->bookmark_list)
	{
	  for (bookmark = page->bookmark_list; bookmark; bookmark = bookmark->next)
	    printf ("bookmark %d '%s'\n", bookmark->level, bookmark->name);
	}
      for (i = page->range.first; i <= page->range.last; i++)
	{
	  printf ("file '%s' ", get_output_file (page->output_context));
	  printf ("format '%s' ", get_output_page_number_format (page->output_context));
	  printf ("page %d\n", i);
	}
    }
}
#endif /* SEMANTIC_DEBUG */

boolean parse_spec_file (char *fn)
{
  boolean result = 0;

  yyin = fopen (fn, "r");
  if (! yyin)
    {
      fprintf (stderr, "can't open spec file '%s'\n", fn);
      goto fail;
    }

  line = 1;

  input_push_context ();  /* create root input context */
  input_push_context ();  /* create inital input context */

  output_push_context ();  /* create root output context */
  output_push_context ();  /* create initial output context */

  yyparse ();

  if (first_input_context->image_count != first_output_context->page_count)
    {
      fprintf (stderr, "input image count %d != output page count %d\n",
	       first_input_context->image_count,
	       first_output_context->page_count);
      goto fail;
    }

  fprintf (stderr, "%d pages specified\n", first_input_context->image_count);

  result = 1;

#ifdef SEMANTIC_DEBUG
  dump_input_tree ();
  dump_output_tree ();
#endif /* SEMANTIC_DEBUG */

 fail:
  if (yyin)
    fclose (yyin);

  return (result);
}
