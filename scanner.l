/*
$Id: scanner.l,v 1.16 2002/08/25 22:02:31 eric Exp $
*/

%option case-insensitive
%option noyywrap

%{
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "semantics.h"
#include "parser.tab.h"

#ifdef SCANNER_DEBUG
#define LDBG(x) printf x
#else
#define LDBG(x)
#endif
%}


digit [0-9]
alpha [a-zA-Z]
dot [\.]

%%

[\,;{}]		{ return (yytext [0]); }
{dot}{dot}	{ LDBG(("elipsis\n")); return (ELIPSIS); }

  /* decimal integer */
{digit}+	{ yylval.integer = atoi (yytext); LDBG(("integer %d\n", yylval.integer)); return (INTEGER); }

  /* floating point number - tricky to make sure it doesn't grab an integer
     followed by an elipsis */
-?{digit}+\.{digit}+ { yylval.fp = atof (yytext); return (FLOAT); }
-?{digit}+\./[^.] { yylval.fp = atof (yytext); return (FLOAT); }

a		{ yylval.size.width = 8.5;
		  yylval.size.height = 11.0;
                  return (PAGE_SIZE); }
b		{ yylval.size.width = 11.0;
                  yylval.size.height = 17.0;
                  return (PAGE_SIZE); }
c		{ yylval.size.width = 17.0;
                  yylval.size.height = 22.0;
                  return (PAGE_SIZE); }
d		{ yylval.size.width = 22.0;
                  yylval.size.height = 34.0;
                  return (PAGE_SIZE); }
e		{ yylval.size.width = 34.0;
                   yylval.size.height = 44.0;
                  return (PAGE_SIZE); }

all		{ return (ALL); }
author		{ return (AUTHOR); }
bookmark	{ return (BOOKMARK); }
cm		{ return (CM); }
creator		{ return (CREATOR); }
crop		{ return (CROP); }
even		{ return (EVEN); }
file		{ return (FILE_KEYWORD); }
image		{ return (IMAGE); }
images		{ return (IMAGES); }
inch		{ return (INCH); }
input		{ return (INPUT); }
keywords	{ return (KEYWORDS); }
label		{ return (LABEL); }
landscape	{ return (LANDSCAPE); }
odd		{ return (ODD); }
output		{ return (OUTPUT); }
page		{ return (PAGE); }
pages		{ return (PAGES); }
portrait	{ return (PORTRAIT) ; }
resolution	{ return (RESOLUTION) ; }
rotate		{ return (ROTATE); }
size		{ return (SIZE); }
subject		{ return (SUBJECT); }
title		{ return (TITLE); }

'[^\n']'	{
		  yylval.character = yytext [1];
		  return (CHARACTER);
		}

\"[^\n"]*\"	{
                  int len = strlen (yytext) - 2;
                  yylval.string = malloc (len + 1);
                  memcpy (yylval.string, yytext + 1, len);
                  yylval.string [len] = '\0';
		  LDBG (("string \"%s\"\n", yylval.string));
                  return (STRING);
                }

[ \t]+		/* whitespace */
\n		{ line++; }

--.*		/* Ada/VHDL style one-line comment */
#.*		/* shell-style one-line comment */

.		{ fprintf (stderr, "Unrecognized character: %s\n", yytext); }

%%
