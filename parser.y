%{
  #include <stdio.h>
  #include "type.h"
  #include "semantics.h"
%}

%union {
  int integer;
  char character;
  double fp;
  char *string;
  page_size_t size;
  range_t range;
  page_label_t page_label;
}

%token <integer> INTEGER
%token <fp> FLOAT
%token <string> STRING
%token <character> CHARACTER
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

%token LABEL
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
	range { input_images ($1); }
	| image_ranges ',' range { input_images ($3); } ;


input_file_clause:
	FILE_KEYWORD STRING  ';'  { input_set_file ($2) } ;

image_clause:
	IMAGE INTEGER ';' { range_t range = { $2, $2 }; input_images (range); } ;

images_clause:
	IMAGES image_ranges ';' ;

rotate_clause:
	ROTATE INTEGER ';' { input_set_rotation ($2) } ;

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

part_clause:
	ODD { input_set_modifier_context (INPUT_MODIFIER_ODD); }
          modifier_clause_list ';'
          { input_set_modifier_context (INPUT_MODIFIER_ALL); }
	| EVEN { input_set_modifier_context (INPUT_MODIFIER_EVEN); }
	  modifier_clause_list ';'
          { input_set_modifier_context (INPUT_MODIFIER_ALL); } ;

input_clause:
	input_file_clause
	| image_clause
	| images_clause
	| part_clause
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
	FILE_KEYWORD STRING  ';' { output_set_file ($2) } ;

label_clause:
	LABEL ';' { page_label_t label = { NULL, '\0' }; output_set_page_label (label); }
	| LABEL STRING ';' { page_label_t label = { $2, '\0' }; output_set_page_label (label); }
	| LABEL CHARACTER ';' { page_label_t label = { NULL, $2 }; output_set_page_label (label); }
	| LABEL STRING ',' CHARACTER ';' { page_label_t label = { $2, $4 }; output_set_page_label (label); } ;

page_ranges:
	range { output_pages ($1); }
	| page_ranges ',' range { output_pages ($3); } ;

page_clause:
	PAGE INTEGER ';' { range_t range = { $2, $2 }; output_pages (range); } ;

pages_clause:
	PAGES page_ranges ';' ;

bookmark_name:
	STRING { output_set_bookmark ($1); } ;

bookmark_name_list:
	bookmark_name
	| bookmark_name_list ',' bookmark_name ;

bookmark_clause:
	BOOKMARK { output_push_context (); bookmark_level++; }
	bookmark_name_list
	output_clause_list ';' { bookmark_level--; output_pop_context (); } ;

output_clause:
	output_file_clause
	| label_clause
	| page_clause | pages_clause
	| bookmark_clause
	| output_clause_list ;

output_clauses:
	output_clause
	| output_clauses output_clause ;

output_clause_list:
	'{' { output_push_context (); }
	output_clauses '}' { output_pop_context (); } ;

output_statement:
	OUTPUT output_clauses ;
