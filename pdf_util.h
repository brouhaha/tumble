void pdf_fatal (char *fmt, ...);

void *pdf_calloc (long int size);

char *pdf_strdup (char *s);

#define pdf_assert(cond) do \
    { \
      if (! (cond)) \
        pdf_fatal ("assert at %s(%d)\n", __FILE__, __LINE__); \
    } while (0)
