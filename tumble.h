typedef struct
{
  bool has_resolution;
  double x_resolution;
  double y_resolution;

  bool has_page_size;
  page_size_t page_size;

  bool has_rotation;
  int rotation;

  bool has_crop;
  crop_t crop;
} input_attributes_t;

bool open_tiff_input_file (char *name);
bool close_tiff_input_file (void);


typedef struct
{
  char *author;
  char *creator;
  char *title;
  char *subject;
  char *keywords;
} pdf_file_attributes_t;

bool open_pdf_output_file (char *name,
			   pdf_file_attributes_t *attributes);


void process_page_numbers (int page_index,
			   int count,
			   int base,
			   page_label_t *page_label);

bool process_page (int image,  /* range 1 .. n */
		   input_attributes_t input_attributes,
		   bookmark_t *bookmarks);
