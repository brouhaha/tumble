%{
#include <stdio.h>
%}

%union {
  int integer;
  double fp;
  char *string;
  struct { double width, double height } size;
}

%token <integer> INTEGER
%token <fp> FLOAT
%token <string> STRING
%token <size> PAGE_SIZE

%token ELIPSIS

%token CM
%token INCH

%token EVEN
%token ODD
%token ALL

%token PORTRAIT
%token LANDSCAPE

%token FILE
%token IMAGE
%token IMAGES
%token ROTATE
%token CROP
%token SIZE
%token INPUT

%token PAGE
%token PAGES
%token BOOKMARK
%token OUTPUT

%type <integer> image_range
%type <integer> image_ranges
%type <integer> page_range
%type <integer> page_ranges

%type <fp> length

%%

statements: 
	statement
	| statements statement ;

statement:
	input_statement
	| output_statement ;


image_range:
	INTEGER ELIPSIS INTEGER
	| INTEGER ;

image_ranges:
	image_range
	| image_ranges ',' image_range ;


file_clause:
	FILE STRING  ';' ;

image_clause:
	IMAGE INTEGER ';'
	| IMAGE INTEGER modifier_clause_list ';' ;

images_clause:
	IMAGES image_ranges ';'
	| IMAGES image_ranges modifier_clause_list ';'
	| IMAGES image_ranges part_clauses ';' ;

rotate_clause:
	ROTATE INTEGER ';' ;

unit:
	CM
	| INCH ;

length:
	FLOAT
	| FLOAT unit ;

crop_clause:
	CROP PAGE_SIZE ';'
	| CROP PAGE_SIZE orientation ';'
	| CROP length ',' length ';'
	| CROP length ',' length ',' length ',' length ';' ;

orientation:
	PORTRAIT
	| LANDSCAPE ;

size_clause:
	SIZE PAGE_SIZE ';'
	| SIZE PAGE_SIZE orientation ';'
	| SIZE length ',' length ';' ;

modifier_clause:
	rotate_clause | crop_clause | size_clause;

modifier_clauses:
	modifier_clause
	| modifier_clauses modifier_clause;

modifier_clause_list:
	'{' modifier_clauses '}' ;

part:
	EVEN | ODD | ALL ;

part_clause:
	part modifier_clause_list;

part_clauses:
	part_clause
	| part_clauses part_clause;

input_clause:
	file_clause
	| image_clause
	| images_clause
	| modifier_clause
	| input_clause_list ;

input_clauses:
	input_clause
	| input_clauses input_clause ;

input_clause_list:
	'{' input_clauses '}' ;

input_statement:
	INPUT input_clauses ;

page_range:
	INTEGER ELIPSIS INTEGER
	| INTEGER ;

page_ranges:
	page_range
	| page_ranges ',' page_range ;

page_clause:
	PAGE INTEGER ';'
	| PAGE STRING ',' INTEGER ';' ;

pages_clause:
	PAGES page_ranges ';'
	| PAGES STRING ',' page_ranges ';' ;

bookmark_clause:
	BOOKMARK INTEGER ',' STRING ';'
	| BOOKMARK STRING ';' ;

output_clause:
	page_clause | pages_clause | bookmark_clause
	| output_clause_list ;

output_clauses:
	output_clause
	| output_clauses output_clause ;

output_clause_list:
	'{' output_clauses '}' ;

output_statement:
	OUTPUT output_clauses ;
