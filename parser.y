/*
 * tumble: build a PDF file from image files
 *
 * Parser
 * Copyright 2001, 2002, 2003, 2017 Eric Smith <spacewar@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.  Note that permission is
 * not granted to redistribute this program under the terms of any
 * other version of the General Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 *
 *  2009-03-13 [JDB] Add support for blank pages, overlay images, color
 *                   mapping, color-key masking, and push/pop of input
 *                   contexts.
 */

%{
  #include <stdbool.h>
  #include <stdint.h>
  #include <stdio.h>
  #include "semantics.h"

  extern int yylex (void);
%}

%define parse.error verbose

%union {
  int integer;
  char character;
  double fp;
  char *string;
  page_size_t size;
  range_t range;
  page_label_t page_label;
  overlay_t overlay;
  rgb_t rgb;
  rgb_range_t rgb_range;
}

%token <integer> INTEGER
%token <fp> FLOAT
%token <string> STRING
%token <character> CHARACTER
%token <size> PAGE_SIZE

%token ELIPSIS

%token CM
%token INCH

%token PORTRAIT
%token LANDSCAPE

%token FILE_KEYWORD
%token IMAGE
%token IMAGES
%token IMAGEMASK
%token ROTATE
%token CROP
%token SIZE
%token RESOLUTION
%token BLANK
%token INPUT

%token TRANSPARENT
%token COLORMAP
%token LABEL
%token OVERLAY
%token PAGE
%token PAGES
%token BOOKMARK
%token OUTPUT

%token AUTHOR
%token CREATOR
%token TITLE
%token SUBJECT
%token KEYWORDS

%type <range> range
%type <range> image_ranges
%type <range> page_ranges

%type <fp> unit

%type <fp> length

%type <integer> orientation

%type <size> page_size

%type <rgb> rgb

%type <rgb_range> rgb_range
%type <rgb_range> gray_range
%type <rgb_range> color_range

%%

statements: 
	statement
	| statements statement ;

statement:
	/* empty: null statement */
	| input_statement
	| output_statement ;


range:
	INTEGER ELIPSIS INTEGER { $$.first = $1; $$.last = $3; }
	| INTEGER { $$.first = $1; $$.last = $1; } ;

image_ranges:
	range { input_images ($1); }
	| image_ranges ',' range { input_images ($3); } ;


input_file_clause:
	FILE_KEYWORD STRING { input_set_file ($2); } ;

image_clause:
	IMAGE INTEGER { range_t range = { $2, $2 }; input_images (range); } ;

images_clause:
	IMAGES image_ranges ;

rotate_clause:
	ROTATE INTEGER { input_set_rotation ($2); } ;

unit:
	/* empty */  /* default to INCH */ { $$ = 1.0; }
	| CM { $$ = (1.0 / 25.4); }
	| INCH { $$ = 1.0; } ;

length:
	FLOAT unit { $$ = $1 * $2; } ;

crop_clause:
	CROP PAGE_SIZE
	| CROP PAGE_SIZE orientation
	| CROP length ',' length
	| CROP length ',' length ',' length ',' length ;

orientation:
	PORTRAIT { $$ = 0; }
	| LANDSCAPE { $$ = 1; } ;

page_size:
	PAGE_SIZE { $$ = $1; }
	| PAGE_SIZE orientation { if ($2)
                                    {
                                      $$.width = $1.height;
				      $$.height = $1.width;
                                    }
                                  else
                                    $$ = $1;
                                }
	| length ',' length { $$.width = $1; $$.height = $3; } ;

size_clause:
	SIZE page_size { input_set_page_size ($2); } ;

resolution_clause:
	RESOLUTION FLOAT unit ;

rgb_range:
	'(' range range range ')' { $$.red = $2; $$.green = $3; $$.blue = $4; } 

gray_range:
	'(' range ')' { $$.red = $2; $$.green = $2; $$.blue = $2; } ;

color_range:
    rgb_range
    | gray_range;

transparency_clause:
    TRANSPARENT color_range { input_set_transparency ($2); } ;

modifier_clause:
	rotate_clause ';'
	| crop_clause ';'
	| size_clause ';'
	| resolution_clause ';'
	| transparency_clause ';' ;

modifier_clauses:
        modifier_clause
	| modifier_clauses modifier_clause ;

blank_page_clause:
	BLANK { input_set_file (NULL); } size_clause;

input_clause:
	input_file_clause ';'
	| image_clause ';'
	| images_clause ';'
	| modifier_clauses ';'
	| blank_page_clause ';'
	| input_clause_list ;

input_clauses:
	input_clause
	| input_clauses input_clause ;

input_clause_list:
	'{' { input_push_context (); }
	input_clauses '}' { input_pop_context (); } ;

input_statement:
	INPUT '{' input_clauses '}' ;

pdf_file_attribute:
	AUTHOR STRING { output_set_author ($2); }
	| CREATOR STRING { output_set_creator ($2); }
	| TITLE STRING { output_set_title ($2); }
	| SUBJECT STRING { output_set_subject ($2); }
	| KEYWORDS STRING { output_set_keywords ($2); } ;

pdf_file_attribute_list:
	pdf_file_attribute
	| pdf_file_attribute_list pdf_file_attribute ;

pdf_file_attributes:
	/* empty */
	| pdf_file_attribute_list ;

output_file_clause:
	FILE_KEYWORD STRING { output_set_file ($2); }
	pdf_file_attributes ;

label_clause:
	LABEL { page_label_t label = { NULL, '\0' }; output_set_page_label (label); }
	| LABEL STRING { page_label_t label = { $2, '\0' }; output_set_page_label (label); }
	| LABEL CHARACTER { page_label_t label = { NULL, $2 }; output_set_page_label (label); }
	| LABEL STRING ',' CHARACTER { page_label_t label = { $2, $4 }; output_set_page_label (label); } ;

overlay_clause_list:
	/* empty */
	| '{' overlay_clauses '}' ;

overlay_clauses:
	overlay_clause ';'
	| overlay_clauses overlay_clause ';' ;

overlay_clause:
	OVERLAY length ',' length { overlay_t overlay = { .imagemask = false,
	    						  .position.x = $2,
							  .position.y = $4 }; output_overlay (overlay); }
	| IMAGEMASK rgb { output_imagemask($2); } ;

page_ranges:
	range { output_pages ($1); }
	| page_ranges ',' range { output_pages ($3); } ;

page_clause:
	PAGE INTEGER { range_t range = { $2, $2 }; output_pages (range); } overlay_clause_list ;

pages_clause:
	PAGES page_ranges ;

bookmark_name:
	STRING { output_set_bookmark ($1); } ;

bookmark_name_list:
	bookmark_name
	| bookmark_name_list ',' bookmark_name ;

bookmark_clause:
	BOOKMARK { output_push_context (); bookmark_level++; }
	bookmark_name_list
	output_clause_list { bookmark_level--; output_pop_context (); } ;

rgb:
	'(' INTEGER INTEGER INTEGER ')' { $$.red = $2; $$.green = $3; $$.blue = $4; } ;

colormap_clause:
	COLORMAP rgb ',' rgb { output_set_colormap ($2, $4); } ;

output_clause:
	output_file_clause ';'
	| colormap_clause ';'
	| label_clause ';'
	| page_clause ';'
	| pages_clause ';'
	| bookmark_clause ';'
	| output_clause_list ;

output_clauses:
	output_clause
	| output_clauses output_clause ;

output_clause_list:
	'{' { output_push_context (); }
	output_clauses '}' { output_pop_context (); } ;

output_statement:
	OUTPUT '{' output_clauses '}' ;
