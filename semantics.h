/*
 * tumble: build a PDF file from image files
 *
 * Semantic routines for spec file parser
 * $Id: semantics.h,v 1.15 2003/03/16 05:58:26 eric Exp $
 * Copyright 2001, 2002, 2003 Eric Smith <eric@brouhaha.com>
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
 */


typedef struct 
{
  double width;
  double height;
} page_size_t;

typedef struct
{
  int first; 
  int last;
 } range_t;

typedef struct
{
  double left;
  double right;
  double top;
  double bottom;
} crop_t;

typedef struct
{
  char *prefix;
  char style;
  /* the following fields are not filled by the parser: */
  int page_index;
  int base;
  int count;
} page_label_t;


typedef enum
{
  INPUT_MODIFIER_ALL,
  INPUT_MODIFIER_ODD,
  INPUT_MODIFIER_EVEN,
  INPUT_MODIFIER_TYPE_COUNT  /* must be last */
} input_modifier_type_t;


typedef struct bookmark_t
{
  struct bookmark_t *next;
  int level;  /* 1 is outermost */
  char *name;
} bookmark_t;


extern int line;  /* line number in spec file */
extern int bookmark_level;


/* semantic routines for input statements */
void input_push_context (void);
void input_pop_context (void);
void input_set_modifier_context (input_modifier_type_t type);
void input_set_file (char *name);
void input_set_rotation (int rotation);
void input_set_page_size (page_size_t size);
void input_images (range_t range);

/* semantic routines for output statements */
void output_push_context (void);
void output_pop_context (void);

void output_set_file (char *name);
void output_set_author (char *author);
void output_set_creator (char *creator);
void output_set_title (char *title);
void output_set_subject (char *subject);
void output_set_keywords (char *keywords);

void output_set_bookmark (char *name);
void output_set_page_label (page_label_t label);
void output_pages (range_t range);


/* functions to be called from main program: */
bool parse_control_file (char *fn);
bool process_controls (void);
