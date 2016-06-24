# Change Log

## [0.35] - 2016-06-23
### Added CHANGELOG.md	
	
## [0.34] - 2016-06-23
### Many fixes and enhancements from Palomar Corporation
- Modify "bitblt.h" to undefine BITS_PER_WORD, which is defined in the
  netpbm header "pm_config.h", so that the tumble use takes precedence.
- Fix a bug in "bitblt_write_g4" (bitblt_g4.c).  Tumble would crash in
  "g4_get_pixel" with bad pointer access when trying to encode a TIFF image
  that was 14400 x 6100 (24" x 11").  "g4_get_pixel" was called with index
  14400, which accesses the first byte beyond the allocated array.
- "bitblt_write_g4" is called to encode and write a TIFF bitmap to the PDF
  file.  It allocates and zeros a temporary buffer to hold a scan line and
  then calls "g4_encode_row" for each line.  "g4_find_pixel" is called to
  find the first 1 bit in the temporary buffer.  Because the buffer has been
  zeroed, "g4_find_pixel" returned the image width (14400) when the
  specified pixel could not be found in scan line.  A subsequent call to
  "g4_get_pixel" specified this pixel number (14400), which accessed memory
  beyond the allocation.

  The crash wasn't repeatable.  To occur, the image width had to be a
  multiple of 32 (else the last pixel + 1 would still be within the final
  word), and the allocation had to end exactly on a page boundary (so the
  first byte beyond the allocation was read-protected).
	
  Fix is to pad the temporary buffer with an extra word if the width is an
  exact multiple of 32.  Note that access beyond the source bitmap is
  prohibited by the "if (a0 >= width) break;" test prior to the
  "g4_get_pixel" call.

- Modify the makefile to add PNG and blank-page support files.

- Modify "parser.y" to enable push and pop of input contexts.  This allows
  "{" and "}" to create a local context for modifiers, such as TRANSPARENT.
  Input modifiers specified outside of a local context become global for
  subsequent input clauses.
- Modify "pdf.c" to add creation and modification dates to the PDF
  properties.  The "CreationDate" and "ModDate" values are set to the time
  that the PDF was created.
- Modify "pdf.h" to change the page size limit from 45 inches (Reader 3.x)
  to 200 inches (Reader 4.x and later), to add structures and parameters to
  support the new features added to the parser, and to add the PDF
  "producer" definition.
- Modify "pdf_g4.c" to add support for color-mapping bilevel images.
  Mapping allows a black and white image to be drawn as any two specified
  colors.  Useful for pages that are black-on-color or white-on-color. The
  color mapping syntax is:

       COLORMAP (<red> <green> <blue>) , (<red> <green> <blue>) ;

  ...where the first triplet specifies the color to be displayed for black
  and the second triplet specifies the color to be displayed for white.

  Note that a two-color PNG is about twice the size of a Group 4 TIFF.

  Implementation note: the colormap is added to the PDF as a string resource
  and an indirect reference is made from the XObject.  Tumble should keep
  each colormap only once in the file by keeping a global list of the
  string resources created.  It doesn't.  However, if the colormap used on
  an image is identical to the one used on the previous image, it will reuse
  the string resource.  This covers the case of a section of colored pages.
  However, a discontiguous run of colored pages, e.g., the front and back
  covers, will store the colormap twice.

- Modify "pdf_g4.c", "pdf_jpeg.c", "tumble.c", "tumble_jpeg.c", and
  "tumble_pbm.c" to support color key masking (using the PDF Mask operator).
  The syntax is:

       TRANSPARENT ( <gray_range> ) ;

  or:

       TRANSPARENT ( <red_range> <green_range> <blue_range> ) ;

  ...as a modifier to an input statement.  <range> may be either a single
  integer or a low..high range.

  The range values must be compatible with the color space of the image.
  For full-color images, the ranges would be 0..255.  For grayscale images,
  the range would be 0..255.  For paletted images, the range  is expressed
  in palette entry numbers, which is much less likely to be useful than a
  single palette value.

- Modify "pdf_g4.c", "tumble_input.h", and "tumble_tiff.c" to add handling
  of TIFF images with min-is-black photometric interpretation.

- Modify "pdf_g4.c," "pdf_jpeg.c," and "pdf_png.c" to fix bugs in the
  callbacks within these files.  "pdf_write_stream" was called to write "Do"
  commands with the XObject names without escaping restricted characters in
  the name, which had been escaped when the corresponding directory was
  written.  This led to "Undefined XObject resource" errors when more than
  26 images were overlaid on a page ("pdf_new_XObject" generates names of
  the form "ImA", "ImB", etc., and the 27th name was "Im[", which must be
  escaped, as "[" is a restricted character; the dictionary name was being
  escaped to "Im#5b", but the "Do" command name was not).

- Modify "pdf_page_label.c" to fix a bug wherein a page label specifying
  prefix without a style (e.g., LABEL <prefix>) produces bad PDF (no labels
  are displayed).  Should output "/P <prefix>" but instead was outputting
  "/S /P <prefix>".

- Add new files "pdf_png.c" and "tumble_png.c" and modify "tumble_input.h"
  to support PNG images (code from Daniel Glöckner via the tumble mailing
  list).

- Change PDF string handling in "pdf_prim.c" and "pdf_prim.h" to allow
  embedded nulls by storing strings as character arrays plus length words.

- Modify "parser.y", "scanner.l", "semantics.c", and "semantics.h" to
  support blank pages, overlay images, color mapping, color-key masking, and
  push/pop of input contexts.

- Modify "tumble.c", "tumble_jpeg.c", and "tumble_pbm.c" to support overlay
  images.  Overlays allow one image to be placed on top of another, e.g., a
  JPEG photo on a bilevel TIFF text page, at a specified position. Overlays
  are drawn after the page on which the overlay is specified.  The overlay
  syntax is:

       PAGE <number> [ { OVERLAY <length>, <length> ;  ... } ] ;

  To support multiple images per page, the "Contents" stream is replaced by
  an array of stream objects.

- Modify "tumble.c" to add a new -V option that prints the program version
  to stdio.  The version string printed is the PDF producer string.

- Add new file "tumble_blank.c" and modify "tumble_input.c" and
  "tumble_input.h" to support true blank pages.  A blank page is a new PDF
  page with no content. The blank page syntax is:

       BLANK <size_clause>

- Fix a bug that caused a double free in "tumble_input.c".

- Modify "tumble_tiff.c" to suppress a "function defined but not used"
  warning.

- Modify "semantics.c" to reduce page label redundancy in the PDF.  Tumble
  was outputting a PageLabel entry for every page, rather than just for
  pages where the labelling changed.  This resulted in an unnecessarily
  large label dictionary.  Now allows page labels to default to
  auto-generation when appropriate.

- Modify "semantics.c" to change the program report from "<n> pages
  specified" to "<n> images specified" because, due to overlay images, the
  number of images does not necessarily equal the number of pages.
