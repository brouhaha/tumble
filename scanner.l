%option case-insensitive

/*
%{
#include "parser.tab.h"
}%
*/

digit [0-9]
alpha [a-zA-Z]

%%

\.\.		{ return (ELIPSIS); }

{digit}+	{ yylval.integer = atoi (yytext); return (INTEGER); }

all		{ return (ALL); }
bookmark	{ return (BOOKMARK); }
crop		{ return (CROP); }
even		{ return (EVEN); }
file		{ return (FILE); }
image		{ return (IMAGE); }
input		{ return (INPUT); }
odd		{ return (ODD); }
output		{ return (OUTPUT); }
page		{ return (PAGE); }
pages		{ return (PAGES); }
rotate		{ return (ROTATE); }
size		{ return (SIZE); }

\".*\"		{ yylval.string = newstr (yytext); return (STRING); }
