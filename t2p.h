typedef struct
{
  page_size_t size;
  int rotation;
  crop_t crop;
} input_attributes_t;

typedef struct
{
  char *page_number;
  bookmark_t *bookmarks;
} output_attributes_t;

boolean open_tiff_input_file (char *name);
boolean close_tiff_input_file (void);

boolean open_pdf_output_file (char *name);
boolean close_pdf_output_file (void);

boolean process_page (int image,  /* range 1 .. n */
		      input_attributes_t input_attributes,
		      output_attributes_t output_attributes);
