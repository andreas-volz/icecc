/* dat.h */

#ifndef _DAT_H_
#define _DAT_H_

#include "scdef.h"

/* one of these should be the last argument to dat_new,
   depending on what type of Dat you want to open */
typedef enum DatType { DAT_FLINGY, DAT_IMAGES, DAT_MAPDATA, DAT_ORDERS, 
			  DAT_SFXDATA, DAT_SPRITES, DAT_TECHDATA, 
			  DAT_UNITS, DAT_UPGRADES, DAT_WEAPONS } DatType;

typedef struct DatFmtEnt {
  size_t   size;   /* size of each var */
  unsigned num;    /* number of this var */
  unsigned offset; /* where varno 0 starts; some variables don't
		      apply for all entries and thus start from a no
		      > 0, but usually this is 0 */
  char     *name;  /* name of this var */
} DatFmtEnt;

/* this structure holds the info about a particular Dat
   file. There are some defined below */
typedef struct DatFmt {
  size_t numvars;            /* number of vars in this format */
  DatFmtEnt entries[64]; /* array of pointers to the var formats,
			        64 should be enough entries. */
} DatFmt;

/* this structure actually holds an entire Dat file */
typedef struct Dat {
  struct DatFmt *fmt; /* a pointer to one of the structures below */
  void   **entries;    /* an array of pointers each Dat entry,
			  these are organized in the same way as in
			  the Dat file: var order, then entry order,
			  check the size of each entry via the fmt pointer
		          to determine what type of array this is */
} Dat;

/* the maximum number of lines we should expect to find
   in a dat entry list */
#define MAX_DATENTLST_LINES 4096
/* the maximum length (in characters) that a name from a dat
   entry list should be expected to contain */
#define MAX_DATENTLST_NAME  512

/* a dat entry list, loaded from a flat file */
typedef struct DatEntLst {
  size_t num;
  char   **strings;
} DatEntLst;

extern void dat_free(Dat *dat_st);
extern Dat *dat_new(const char *dat_file_name, DatType type);
extern int dat_save(char *file_name, Dat *dat_st);
extern uint32 dat_get_value(const Dat *dat_st, 
				   unsigned entry, unsigned var);
extern uint32 dat_get_value_by_varname(const Dat *dat_st, 
				    unsigned entry, const char *name);
extern unsigned dat_indexof_varname(const Dat *dat_st, char *name);
extern char *dat_nameof_varno(const Dat *dat_st, unsigned var);
extern size_t dat_sizeof_varno(const Dat *dat_st, unsigned var);
extern unsigned dat_numberof_varno(const Dat *dat_st, unsigned var);
extern unsigned dat_offsetof_varno(const Dat *dat_st, unsigned var);
extern size_t dat_numberof_vars(const Dat *dat_st);
extern size_t dat_numberof_entries(const Dat *dat_st);
extern int dat_isvalid_varno(const Dat *dat_st, unsigned var);
extern int dat_isvalid_entryno(const Dat *dat_st, unsigned entry, unsigned var);
extern void dat_set_value(Dat *dat_st, unsigned entry, unsigned var, uint32 newval);

extern DatEntLst *datentlst_new(char *file_name);
extern void datentlst_free(DatEntLst *datlist);
extern char *datentlst_get_string(DatEntLst *datlist, int index);
extern size_t datentlst_numberof_strings(DatEntLst *datlist);

#endif
