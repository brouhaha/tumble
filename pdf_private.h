struct pdf_page
{
  pdf_file_handle pdf_file;
  struct pdf_obj *page_dict;
  struct pdf_obj *media_box;
  struct pdf_obj *procset;
  struct pdf_obj *resources;

  char last_XObject_name;
  struct pdf_obj *XObject_dict;
};


struct pdf_pages
{
  struct pdf_obj *pages_dict;
  struct pdf_obj *kids;
  struct pdf_obj *count;
};


struct pdf_file
{
  FILE             *f;
  struct pdf_obj   *first_ind_obj;
  struct pdf_obj   *last_ind_obj;
  long int         xref_offset;
  struct pdf_obj   *catalog;
  struct pdf_obj   *info;
  struct pdf_pages *root;
  struct pdf_obj   *trailer_dict;
};
