/*
$Id: scanner.l,v 1.7 2001/12/29 17:54:43 eric Exp $
*/

%option case-insensitive
%option noyywrap

%{
#include <string.h>
#include "parser.tab.h"
%}


digit [0-9]
alpha [a-zA-Z]

%%

\.\.		{ return (ELIPSIS); }

{digit}+	{ yylval.integer = atoi (yytext); return (INTEGER); }
{digit}+.{digit}* { yylval.fp = atof (yytext); return (FLOAT); }

a		{ yylval.size.width = 8.5 * 25.4;
		  yylval.size.height = 11.0 * 25.4;
                  return (PAGE_SIZE); }
b		{ yylval.size.width = 11.0 * 25.4;
                  yylval.size.height = 17.0 * 25.4;
                  return (PAGE_SIZE); }
c		{ yylval.size.width = 17.0 * 25.4;
                  yylval.size.height = 22.0 * 25.4;
                  return (PAGE_SIZE); }
d		{ yylval.size.width = 22.0 * 25.4;
                  yylval.size.height = 34.0 * 25.4;
                  return (PAGE_SIZE); }
e		{ yylval.size.width = 34.0 * 25.4;
                   yylval.size.height = 44.0 * 25.4;
                  return (PAGE_SIZE); }

all		{ return (ALL); }
bookmark	{ return (BOOKMARK); }
cm		{ return (CM); }
crop		{ return (CROP); }
even		{ return (EVEN); }
file		{ return (FILE_KEYWORD); }
image		{ return (IMAGE); }
images		{ return (IMAGES); }
inch		{ return (INCH); }
input		{ return (INPUT); }
landscape	{ return (LANDSCAPE); }
odd		{ return (ODD); }
output		{ return (OUTPUT); }
page		{ return (PAGE); }
pages		{ return (PAGES); }
portrait	{ return (PORTRAIT) ; }
resolution	{ return (RESOLUTION) ; }
rotate		{ return (ROTATE); }
size		{ return (SIZE); }

\".*\"		{ yylval.string = strdup (yytext); return (STRING); }

[ \t\n]+	/* whitespace */

--.*		/* Ada/VHDL style one-line comment */

.		{ printf( "Unrecognized character: %s\n", yytext ); }

%%
