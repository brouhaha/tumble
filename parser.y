%{
#include <stdio.h>
%}

%union {
  int integer;
  double fp;
  char *string;
}

%token <integer> INTEGER
%token <fp> FLOAT
%token <string> STRING

%token EVEN
%token ODD
%token ALL

%token FILE
%token IMAGE
%token ROTATE
%token CROP
%token SIZE
%token INPUT

%token PAGE
%token BOOKMARK
%token OUTPUT

%type <integer> range
%type <integer> ranges

%%

statements: statement | statements statement;

statement: input_statement | output_statement;


range:	INTEGER ".." INTEGER
	| INTEGER;

ranges:	range
	| ranges ',' range;


file_clause:
	FILE STRING  ';' ;

image_clause:
	IMAGE ranges ';' ;

rotate_clause:
	ROTATE INTEGER ';' ;

crop_clause:
	CROP FLOAT ',' FLOAT ';' ;

size_clause:
	SIZE FLOAT ',' FLOAT ';' ;

part:
	EVEN | ODD | ALL;

part_clause:
	part ';' ;

input_clause:
	part_clause
	| file_clause | image_clause | rotate_clause
	| crop_clause | size_clause;

input_clauses:
	input_clause
	| input_clauses input_clause;

input_statement:
	INPUT '{' input_clauses '}' ;


page_clause:
	PAGE ranges ';'
	| PAGE ranges ',' STRING ';' ;

bookmark_clause:
	BOOKMARK STRING ';' ;

output_clause:
	page_clause | bookmark_clause;

output_clauses:
	output_clause
	| output_clauses output_clause;

output_statement:
	OUTPUT '{' output_clauses '}' ;

