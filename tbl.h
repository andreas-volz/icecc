/* Tbl.h */

#ifndef _TBL_H_
#define _TBL_H_

#include "scdef.h"

/* we'll use a buffer of this size to copy each string
   from the file into. Make bigger/smaller as needed.
   (the amount allocated on the heap for each string is
   exact fit; this is just the temp buffer size */
#define TBL_STRING_MAX_LEN 256

/* abstraction of a Tbl file, remember that Tbl files start
   indexing from 1, not 0 (this is the same way I index it.
   size still refers to the number of entries (so use
   <=size and not <size in for loops to get all the entries */
typedef struct Tbl {
  uint16 size;     /* number of strings in the Tbl file */
  char** entries; /* heap allocated array of the strings */
} Tbl;

extern void tbl_free(Tbl *tbl_st);
extern Tbl *tbl_new(char *tbl_file_name);
extern int tbl_save(char *file_name, Tbl *tbl_st);
extern char * tbl_get_string(Tbl *tbl_st, unsigned i);
extern unsigned tbl_get_size(Tbl *tbl_st);
extern void tbl_set_string(Tbl *tbl_st, unsigned i, const char *newstr);
extern void tbl_insert_string(Tbl *tbl_st, unsigned i, const char *newstr);

#endif
