#include <stdio.h>
#include <string.h>


#include "libpdf.h"
#include "libpdf_util.h"
#include "libpdf_prim.h"
#include "libpdf_private.h"


struct pdf_g4_image
{
  unsigned long Columns;
  unsigned long Rows;
  unsigned long rowbytes;
  int BlackIs1;
  unsigned char *data;
  unsigned long len;
  char XObject_name [4];
};


char pdf_new_XObject (pdf_page_handle pdf_page, struct pdf_obj *ind_ref)
{
  char XObject_name [4] = "Im ";

  XObject_name [2] = ++pdf_page->last_XObject_name;
  
  if (! pdf_page->XObject_dict)
    {
      pdf_page->XObject_dict = pdf_new_obj (PT_DICTIONARY);
      pdf_set_dict_entry (pdf_page->resources, "XObject", pdf_page->XObject_dict);
    }

  pdf_set_dict_entry (pdf_page->XObject_dict, & XObject_name [0], ind_ref);

  return (pdf_page->last_XObject_name);
}


void pdf_write_g4_content_callback (pdf_file_handle pdf_file,
				    struct pdf_obj *stream,
				    void *app_data)
{
  unsigned long width = (8.5 * 72);  /* full width of page */
  unsigned long height = (11 * 72);  /* full height of page */
  unsigned long x = 0;  /* 0 is left edge */
  unsigned long y = 0;  /* 0 is bottom edge */
  struct pdf_g4_image *image = app_data;

  char str1 [100];
  char *str2 = "/";
  char *str3 = " Do\r\n";

  /* width 0 0 height x y cm */
  sprintf (str1, "q %ld 0 0 %ld %ld %ld cm\r\n", width, height, x, y);

  pdf_stream_write_data (pdf_file, stream, str1, strlen (str1));
  pdf_stream_write_data (pdf_file, stream, str2, strlen (str2));
  pdf_stream_write_data (pdf_file, stream, & image->XObject_name [0],
			 strlen (& image->XObject_name [0]));
  pdf_stream_write_data (pdf_file, stream, str3, strlen (str3));
}


void pdf_write_g4_fax_image_callback (pdf_file_handle pdf_file,
				      struct pdf_obj *stream,
				      void *app_data)
{
  struct pdf_g4_image *image = app_data;

#if 1
  pdf_stream_write_data (pdf_file, stream, image->data, image->len);
#else
  unsigned long row = 0;
  unsigned char *ref;
  unsigned char *raw;

  ref = NULL;
  raw = image->data;

  while (row < image->Rows)
    {
      pdf_stream_write_data (pdf_file, stream, raw, image->rowbytes);

      row++;
      ref = raw;
      raw += image->rowbytes;
    }
  /* $$$ generate and write EOFB code */
  /* $$$ flush any remaining buffered bits */
#endif
}


void pdf_write_g4_fax_image (pdf_page_handle pdf_page,
			     unsigned long Columns,
			     unsigned long Rows,
			     unsigned long rowbytes,
			     int ImageMask,
			     int BlackIs1,          /* boolean, typ. false */
			     unsigned char *data,
			     unsigned long len)
{
  struct pdf_g4_image *image;

  struct pdf_obj *stream;
  struct pdf_obj *stream_dict;
  struct pdf_obj *decode_parms;

  struct pdf_obj *content_stream;

  image = pdf_calloc (sizeof (struct pdf_g4_image));

  image->Columns = Columns;
  image->Rows = Rows;
  image->rowbytes = rowbytes;
  image->BlackIs1 = BlackIs1;
  image->data = data;
  image->len = len;

  stream_dict = pdf_new_obj (PT_DICTIONARY);

  stream = pdf_new_ind_ref (pdf_page->pdf_file,
			    pdf_new_stream (pdf_page->pdf_file,
					    stream_dict,
					    & pdf_write_g4_fax_image_callback,
					    image));

  strcpy (& image->XObject_name [0], "Im ");
  image->XObject_name [2] = pdf_new_XObject (pdf_page, stream);

  pdf_set_dict_entry (stream_dict, "Type",    pdf_new_name ("XObject"));
  pdf_set_dict_entry (stream_dict, "Subtype", pdf_new_name ("Image"));
  pdf_set_dict_entry (stream_dict, "Name",    pdf_new_name (& image->XObject_name [0]));
  pdf_set_dict_entry (stream_dict, "Width",   pdf_new_integer (Columns));
  pdf_set_dict_entry (stream_dict, "Height",  pdf_new_integer (Rows));
  pdf_set_dict_entry (stream_dict, "BitsPerComponent", pdf_new_integer (1));
  if (ImageMask)
    pdf_set_dict_entry (stream_dict, "ImageMask", pdf_new_bool (ImageMask));
  else
    pdf_set_dict_entry (stream_dict, "ColorSpace", pdf_new_name ("DeviceGray"));

  decode_parms = pdf_new_obj (PT_DICTIONARY);

  pdf_set_dict_entry (decode_parms,
		      "K",
		      pdf_new_integer (-1));

  pdf_set_dict_entry (decode_parms,
		      "Columns",
		      pdf_new_integer (Columns));

  pdf_set_dict_entry (decode_parms,
		      "Rows",
		      pdf_new_integer (Rows));

  if (BlackIs1)
    pdf_set_dict_entry (decode_parms,
			"BlackIs1",
			pdf_new_bool (BlackIs1));

  pdf_stream_add_filter (stream, "CCITTFaxDecode", decode_parms);

  /* the following will write the stream, using our callback function to
     get the actual data */
  pdf_write_ind_obj (pdf_page->pdf_file, stream);

  content_stream = pdf_new_ind_ref (pdf_page->pdf_file,
				    pdf_new_stream (pdf_page->pdf_file,
						    pdf_new_obj (PT_DICTIONARY),
						    & pdf_write_g4_content_callback,
						    image));

  pdf_set_dict_entry (pdf_page->page_dict, "Contents", content_stream);

  pdf_write_ind_obj (pdf_page->pdf_file, content_stream);
}

