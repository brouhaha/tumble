#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "type.h"
#include "semantics.h"
#include "parser.tab.h"


#define SEMANTIC_DEBUG
#ifdef SEMANTIC_DEBUG
#define SDBG(x) printf x
#else
#define SDBG(x)
#endif


FILE *yyin;
int line;  /* line number in spec file */


int input_page_count;   /* total input pages in spec */
int output_page_count;  /* total output pages in spec */


input_context_t *current_input_context;
input_modifier_type_t current_modifier_context;


void input_push_context (void)
{
  input_context_t *new_input_context;

  new_input_context = malloc (sizeof (input_context_t));
  if (! new_input_context)
    {
      fprintf (stderr, "failed to calloc an input context\n");
      return;
    }

  if (current_input_context)
    {
      memcpy (new_input_context, current_input_context, sizeof (input_context_t));
      new_input_context->page_count = 0;
    }
  else
    memset (new_input_context, 0, sizeof (input_context_t));

  new_input_context->parent_input_context = current_input_context;
  current_input_context = new_input_context;
};

void input_pop_context (void)
{
  if (! current_input_context)
    {
      fprintf (stderr, "failed to pop an input context\n");
      return;
    }

  current_input_context = current_input_context->parent_input_context;
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

void input_set_file (char *name)
{
};

void input_set_rotation (int rotation)
{
  current_input_context->modifiers [current_modifier_context].has_rotation = 1;
  current_input_context->modifiers [current_modifier_context].rotation = rotation;
  SDBG(("rotation %d\n", rotation));
}

void input_images (int first, int last)
{
  input_page_count += ((last - first) + 1);
#ifdef SEMANTIC_DEBUG
  if (first == last)
    SDBG(("image %d\n", first));
  else
    SDBG(("images %d..%d\n", first, last));
#endif /* SEMANTIC_DEBUG */
}


void output_push_context (void)
{
};

void output_set_file (char *name)
{
};

void output_pages (int first, int last)
{
  output_page_count += ((last - first) + 1);
#ifdef SEMANTIC_DEBUG
  if (first == last)
    SDBG(("page %d\n", first));
  else
    SDBG(("pages %d..%d\n", first, last));
#endif /* SEMANTIC_DEBUG */
}


void yyerror (char *s)
{
  fprintf (stderr, "%d: %s\n", line, s);
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

  input_push_context ();  /* create initial input context */
  output_push_context ();  /* create initial output context */

  yyparse ();

  if (input_page_count != output_page_count)
    {
      fprintf (stderr, "input page count %d != output page count %d\n",
	       input_page_count, output_page_count);
      goto fail;
    }

  fprintf (stderr, "%d pages specified\n", input_page_count);

  result = 1;

 fail:
  if (yyin)
    fclose (yyin);

  return (result);
}
