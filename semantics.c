#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "type.h"
#include "semantics.h"
#include "parser.tab.h"
#include "t2p.h"


typedef struct
{
  boolean has_page_size;
  page_size_t page_size;

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
  pdf_file_attributes_t file_attributes;

  bookmark_t *first_bookmark;
  bookmark_t *last_bookmark;

  boolean has_page_label;
  page_label_t page_label;
} output_context_t;


typedef struct output_page_t
{
  struct output_page_t *next;
  output_context_t *output_context;
  range_t range;
  bookmark_t *bookmark_list;
} output_page_t;


#undef SEMANTIC_DEBUG

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
  last_input_context = new_input_context;
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

void input_set_page_size (page_size_t size)
{
  last_input_context->modifiers [current_modifier_context].has_page_size = 1;
  last_input_context->modifiers [current_modifier_context].page_size = size;
  SDBG(("page size %f, %f\n", size.width, size.height));
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
  last_output_context->file_attributes.author = NULL;
  last_output_context->file_attributes.creator = NULL;
  last_output_context->file_attributes.title = NULL;
  last_output_context->file_attributes.subject = NULL;
  last_output_context->file_attributes.keywords = NULL;
};

void output_set_author (char *author)
{
  last_output_context->file_attributes.author = author;
}

void output_set_creator (char *creator)
{
  last_output_context->file_attributes.creator = creator;
}

void output_set_title (char *title)
{
  last_output_context->file_attributes.title = title;
}

void output_set_subject (char *subject)
{
  last_output_context->file_attributes.subject = subject;
}

void output_set_keywords (char *keywords)
{
  last_output_context->file_attributes.keywords = keywords;
}

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

void output_set_page_label (page_label_t label)
{
  output_clone ();
  last_output_context->has_page_label = 1;
  last_output_context->page_label = label;
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

static boolean get_input_rotation (input_context_t *context,
				   input_modifier_type_t type,
				   int *rotation)
{
  for (; context; context = context->parent)
    {
      if (context->modifiers [type].has_rotation)
	{
	  * rotation = context->modifiers [type].rotation;
	  return (1);
	}
      if (context->modifiers [INPUT_MODIFIER_ALL].has_rotation)
	{
	  * rotation = context->modifiers [INPUT_MODIFIER_ALL].rotation;
	  return (1);
	}
    }
  return (0);  /* default */
}

static boolean get_input_page_size (input_context_t *context,
				    input_modifier_type_t type,
				    page_size_t *page_size)
{
  for (; context; context = context->parent)
    {
      if (context->modifiers [type].has_page_size)
	{
	  * page_size = context->modifiers [type].page_size;
	  return (1);
	}
      if (context->modifiers [INPUT_MODIFIER_ALL].has_page_size)
	{
	  * page_size = context->modifiers [INPUT_MODIFIER_ALL].page_size;
	  return (1);
	}
    }
  return (0);  /* default */
}

static char *get_output_file (output_context_t *context)
{
  for (; context; context = context->parent)
    if (context->output_file)
      return (context->output_file);
  fprintf (stderr, "no output file found\n");
  exit (2);
}

static pdf_file_attributes_t *get_output_file_attributes (output_context_t *context)
{
  for (; context; context = context->parent)
    if (context->output_file)
      return (& context->file_attributes);
  fprintf (stderr, "no output file found\n");
  exit (2);
}

static page_label_t *get_output_page_label (output_context_t *context)
{
  for (; context; context = context->parent)
    if (context->has_page_label)
      return (& context->page_label);
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
	boolean has_rotation, has_page_size;
	int rotation;
	page_size_t page_size;

	has_rotation = get_input_rotation (image->input_context,
					   parity,
					   & rotation);
	has_page_size = get_input_page_size (image->input_context,
					     parity,
					     & page_size);
	printf ("file '%s' image %d",
	        get_input_file (image->input_context),
		i);
	if (has_rotation)
	  printf (" rotation %d", rotation);
	if (has_page_size)
	  printf (" size %f, %f", page_size.width, page_size.height);
	printf ("\n");
	printf ("context: %08x\n", image->input_context);
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
	    printf ("bookmark %d \"%s\"\n", bookmark->level, bookmark->name);
	}
      for (i = page->range.first; i <= page->range.last; i++)
	{
	  page_label_t *label = get_output_page_label (page->output_context);
	  printf ("file \"%s\" ", get_output_file (page->output_context));
	  if (label)
	    {
	      printf ("label ");
	      if (label->prefix)
		printf ("\"%s\" ", label->prefix);
	      if (label->style)
		printf ("'%c' ", label->style);
	    }
	  printf ("page %d\n", i);
	}
    }
}
#endif /* SEMANTIC_DEBUG */


static inline int range_count (range_t range)
{
  return ((range.last - range.first) + 1);
}


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


boolean process_specs (void)
{
  input_image_t *image = NULL;
  output_page_t *page = NULL;
  int i = 0;
  int p = 0;
  int page_index = 0;
  input_attributes_t input_attributes;
  input_modifier_type_t parity;
  page_label_t *page_label;

  for (;;)
    {
      if ((! image) || (i >= range_count (image->range)))
	{
	  if (image)
	    image = image->next;
	  else
	    image = first_input_image;
	  if (! image)
	    return (0);
	  i = 0;
	  if (! open_tiff_input_file (get_input_file (image->input_context)))
	    return (0);
	}

      if ((! page) || (p >= range_count (page->range)))
	{
	  if (page)
	    page = page->next;
	  else
	    page = first_output_page;
	  p = 0;
	  if (! open_pdf_output_file (get_output_file (page->output_context),
				      get_output_file_attributes (page->output_context)))
	    return (0);
	  page_label = get_output_page_label (page->output_context);
	  process_page_numbers (page_index,
				range_count (page->range),
				page->range.first,
				page_label);
	}

      parity = ((image->range.first + i) % 2) ? INPUT_MODIFIER_ODD : INPUT_MODIFIER_EVEN;

      memset (& input_attributes, 0, sizeof (input_attributes));

      input_attributes.rotation = 0;
      input_attributes.has_rotation = get_input_rotation (image->input_context,
							  parity,
							  & input_attributes.rotation);

      input_attributes.has_page_size = get_input_page_size (image->input_context,
							    parity,
							    & input_attributes.page_size);

      process_page (image->range.first + i,
		    input_attributes,
		    page->bookmark_list);
      i++;
      p++;
      page_index++;
    }

  return (1);
}
