%{
  #include <stdio.h>
  #include "type.h"
  #include "tiff2pdf.h"
%}

%union {
  int integer;
  double fp;
  char *string;
  struct { double width; double height; } size;
  struct { int first; int last; } range;
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

%token FILE_KEYWORD
%token IMAGE
%token IMAGES
%token ROTATE
%token CROP
%token SIZE
%token RESOLUTION
%token INPUT

%token PAGE
%token PAGES
%token BOOKMARK
%token OUTPUT

%type <range> range
%type <range> image_ranges
%type <range> page_ranges

%type <fp> unit



%type <fp> length

%%

statements: 
	statement
	| statements statement ;

statement:
	input_statement
	| output_statement ;


range:
	INTEGER ELIPSIS INTEGER { $$.first = $1; $$.last = $3; }
	| INTEGER { $$.first = $1; $$.last = $1; } ;

image_ranges:
	range { input_images ($1.first, $1.last); }
	| image_ranges ',' range { input_images ($3.first, $3.last); } ;


input_file_clause:
	FILE_KEYWORD STRING  ';'  { open_tiff_input_file ($2) } ;

image_clause:
	IMAGE INTEGER ';' { input_images ($2, $2); }
	| IMAGE INTEGER modifier_clause_list ';' { input_images ($2, $2); } ;

images_clause:
	IMAGES image_ranges ';'
	| IMAGES image_ranges modifier_clause_list ';'
	| IMAGES image_ranges part_clauses ';' ;

rotate_clause:
	ROTATE INTEGER ';' ;

unit:
	/* empty */  /* default to INCH */ { $$ = 25.4; }
	| CM { $$ = 1.0; }
	| INCH { $$ = 25.4; } ;

length:
	FLOAT unit { $$ = $1 * $2; } ;

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

resolution_clause:
	RESOLUTION FLOAT unit ;

modifier_clause:
	rotate_clause | crop_clause | size_clause | resolution_clause;

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
	input_file_clause
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

output_file_clause:
	FILE_KEYWORD STRING  ';' { open_pdf_output_file ($2) } ;

page_ranges:
	range { output_pages ($1.first, $1.last); }
	| page_ranges ',' range { output_pages ($3.first, $3.last); } ;

page_clause:
	PAGE INTEGER ';' { output_pages ($2, $2); }
	| PAGE STRING ',' INTEGER ';' { output_pages ($4, $4); } ;

pages_clause:
	PAGES page_ranges ';'
	| PAGES STRING ',' page_ranges ';' ;

bookmark_clause:
	BOOKMARK INTEGER ',' STRING ';'
	| BOOKMARK STRING ';' ;

output_clause:
	output_file_clause
	| page_clause | pages_clause
	| bookmark_clause
	| output_clause_list ;

output_clauses:
	output_clause
	| output_clauses output_clause ;

output_clause_list:
	'{' output_clauses '}' ;

output_statement:
	OUTPUT output_clauses ;
