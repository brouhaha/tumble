#include <stdio.h>

#include "type.h"
#include "parser.tab.h"
#include "semantics.h"


FILE *yyin;
int line;  /* line number in spec file */


int input_count;   /* total input pages in spec */
int output_count;  /* total output pages in spec */


void input_push_context (input_context_type_t type)
{
};

void input_pop_context (void)
{
};

void input_set_file (char *name)
{
};

void input_images (int first, int last)
{
  input_count += ((last - first) + 1);
  if (first == last)
    printf ("image %d\n", first);
  else
    printf ("iamges %d..%d\n", first, last);
}

void output_set_file (char *name)
{
};

void output_pages (int first, int last)
{
  output_count += ((last - first) + 1);
  if (first == last)
    printf ("page %d\n", first);
  else
    printf ("pages %d..%d\n", first, last);
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

  yyparse ();

  result = 1;

 fail:
  if (yyin)
    fclose (yyin);

  return (result);
}
