typedef enum
{
  PT_BAD,

  /* scalar */
  PT_NULL,
  PT_BOOL,
  PT_NAME,
  PT_STRING,
  PT_INTEGER,
  PT_REAL,
  PT_IND_REF,

  /* composite */
  PT_DICTIONARY,
  PT_ARRAY,
  PT_STREAM
} pdf_obj_type;


struct pdf_obj;


typedef void (*pdf_stream_write_callback)(pdf_file_handle pdf_file,
					  struct pdf_obj *stream,
					  void *app_data);


void pdf_set_dict_entry (struct pdf_obj *dict_obj, char *key, struct pdf_obj *val);
struct pdf_obj *pdf_get_dict_entry (struct pdf_obj *dict_obj, char *key);


void pdf_add_array_elem (struct pdf_obj *array_obj, struct pdf_obj *val);


/* Create a new object that will NOT be used indirectly */
struct pdf_obj *pdf_new_obj (pdf_obj_type type);

struct pdf_obj *pdf_new_bool (int bool);

struct pdf_obj *pdf_new_name (char *name);

struct pdf_obj *pdf_new_string (char *str);

struct pdf_obj *pdf_new_integer (unsigned long val);

struct pdf_obj *pdf_new_real (double val);


/* Create a new indirect object */
struct pdf_obj *pdf_new_ind_ref (pdf_file_handle pdf_file, struct pdf_obj *obj);

/* get the object referenced by an indirect reference */
struct pdf_obj *pdf_deref_ind_obj (struct pdf_obj *ind_obj);


unsigned long pdf_get_integer (struct pdf_obj *obj);
void pdf_set_integer (struct pdf_obj *obj, unsigned long val);


double pdf_get_real (struct pdf_obj *obj);
void pdf_set_real (struct pdf_obj *obj, double val);


/* The callback will be called when the stream data is to be written to the
   file.  app_data will be passed as an argument to the callback. */
struct pdf_obj *pdf_new_stream (pdf_file_handle pdf_file,
				struct pdf_obj *stream_dict,
				pdf_stream_write_callback callback,
				void *app_data);

/* The callback should call pdf_stream_write_data to write the actual
   stream data. */
void pdf_stream_write_data (pdf_file_handle pdf_file,
			    struct pdf_obj *stream,
			    char *data,
			    unsigned long len);

void pdf_stream_add_filter (struct pdf_obj *stream,
			    char *filter_name,
			    struct pdf_obj *decode_parms);


/* Write the object to the file */
void pdf_write_obj (pdf_file_handle pdf_file, struct pdf_obj *obj);


/* Write the indirect object to the file.  For most objects this should
   be done by pdf_write_all_ind_obj() when the file is being closed, but for
   large objects such as streams, it's probably better to do it as soon as the
   object is complete. */
void pdf_write_ind_obj (pdf_file_handle pdf_file, struct pdf_obj *ind_obj);


/* Write all indirect objects that haven't already been written to the file. */
void pdf_write_all_ind_obj (pdf_file_handle pdf_file);


/* Write the cross reference table, and return the maximum object number */
unsigned long pdf_write_xref (pdf_file_handle pdf_file);
