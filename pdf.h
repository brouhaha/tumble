typedef struct pdf_file *pdf_file_handle;

typedef struct pdf_page *pdf_page_handle;


void pdf_init (void);

pdf_file_handle pdf_create (char *filename);

void pdf_close (pdf_file_handle pdf_file);

void pdf_set_author   (pdf_file_handle pdf_file, char *author);
void pdf_set_creator  (pdf_file_handle pdf_file, char *author);
void pdf_set_title    (pdf_file_handle pdf_file, char *author);
void pdf_set_subject  (pdf_file_handle pdf_file, char *author);
void pdf_set_keywords (pdf_file_handle pdf_file, char *author);


/* width and height in units of 1/72 inch */
pdf_page_handle pdf_new_page (pdf_file_handle pdf_file,
			      double width,
			      double height);

void pdf_close_page (pdf_page_handle pdf_page);


/* The length of the data must be Rows * rowbytes.
   Note that rowbytes must be at least (Columns+7)/8, but may be arbitrarily
   large. */
void pdf_write_g4_fax_image (pdf_page_handle pdf_page,
			     unsigned long Columns,
			     unsigned long Rows,
			     unsigned long rowbytes,
			     int ImageMask,
			     int BlackIs1,          /* boolean, typ. false */
			     unsigned char *data,
			     unsigned long len);


void pdf_set_page_number (pdf_page_handle pdf_page, char *page_number);

void pdf_bookmark (pdf_page_handle pdf_page, int level, char *name);

void pdf_insert_tiff_image (pdf_page_handle pdf_page, char *filename);
