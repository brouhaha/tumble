extern int line;

boolean open_tiff_input_file (char *name);
boolean close_tiff_input_file (void);

boolean open_pdf_output_file (char *name);
boolean close_pdf_output_file (void);

boolean process_page (int image);  /* range 1 .. n */

void input_images (int first, int last);
void output_pages (int first, int last);







