/* scdef.h */

#ifndef _SCDEF_H_
#define _SCDEF_H_

/* --- System Dependent Configuration Variables --- */

typedef unsigned long  uint32; /* must be 32 bits wide & unsigned */
typedef unsigned short uint16; /* must be 16 bits wide & unsigned */
typedef unsigned char  byte;   /* must be 8 bits wide & unsigned */

/* The character string which is used to seperate directory strings:
   "/" on UNIX, "\\" on Windows, ":" on Mac, except OS X, which is a unix */
#if defined(WIN32)
#define DIR_SEPARATOR "\\"
#elif defined(MACOS)
#define DIR_SEPARATOR ":"
#else
#define DIR_SEPARATOR  "/"
#endif

/* big endian macs */
#if defined(MACOS)
#define REVERSE_ENDIAN
#elif defined(MACOSX)
#define REVERSE_ENDIAN
#endif

/* ------------------------------------------------ */

/* #define __DEBUG */
/* #define __DEBUG_MEMORY */

typedef uint16         addr;   /* SC uses 16-bit addresses mainly */

#define EOF_UINT16     ((uint16)0xFFFF)
#define NULL_UINT16    ((uint16)0x0000)

#include <stdlib.h>
#include <stdio.h>

#ifdef __DEBUG_MEMORY

#include <time.h>
#include <math.h>

/* #define MEMORY_WARN_ON_HASH_COLLISION */

#define die(msg,filename,lineno) \
{ fprintf(stderr,"died at %s:%d - %s\n",(filename),(lineno),(msg)); exit(1); }

#define malloc(sz)     __my_malloc((sz),__FILE__,__LINE__)
#define calloc(num,sz) __my_calloc((num),(sz),__FILE__,__LINE__)
#define free(ptr)      __my_free((ptr),__FILE__,__LINE__)

extern void print_memory_leaks() ;
extern void *__my_malloc(int sz, char *filename, int lineno);
extern void *__my_calloc(int num, int sz, char *filename, int lineno);
extern void  __my_free(void *ptr, char *filename, int lineno);

#else

/* refefine memory allocation to do a dummy allocation of one byte if
   we ask for a pointer to nothing. Mac OS X doesn't seem to like asking
   for nothing. */
#define malloc(sz)     ((sz)==0?NULL:malloc(sz))
#define calloc(num,sz) ((num)*(sz)==0?NULL:calloc(num,sz))

#endif

/* Memory mapped file for quick reads (avoid disk IO if
   we have to read the entire file anyway). */
typedef struct MFILE {
  long pos;    /* current position in file */
  long size;   /* total size of file in bytes */
  byte *array; /* the array that holds the file */

#ifdef __DEBUG
  byte *visited; /* for debugging, this will keep track of the
		    bytes that have been read or written on */
#endif

} MFILE;

/* version of file; right now only differentiated for iscript
   files... need to work on backward compatibility for Dat files */
enum version { BROODWAR=0, STARCRAFT=1 };

/* these are standard definitions for all sc type files */
#include "list.h"
#include "queue.h"
#include "hashtable.h"
#include "dat.h"
#include "tbl.h"
#include "iscript.h"

/* Define some paths to tell us where to find the default files */
#define ISCRIPT_BIN_FILE_PATH "scripts" DIR_SEPARATOR "iscript.bin"
#define IMAGES_DAT_FILE_PATH  "arr" DIR_SEPARATOR "images.dat"
#define FLINGY_DAT_FILE_PATH  "arr" DIR_SEPARATOR "flingy.dat"
#define SFXDATA_DAT_FILE_PATH "arr" DIR_SEPARATOR "sfxdata.dat"
#define SPRITES_DAT_FILE_PATH "arr" DIR_SEPARATOR "sprites.dat"
#define UNITS_DAT_FILE_PATH   "arr" DIR_SEPARATOR "units.dat"
#define WEAPONS_DAT_FILE_PATH "arr" DIR_SEPARATOR "weapons.dat"

#define STAT_TXT_TBL_FILE_PATH "rez" DIR_SEPARATOR "stat_txt.tbl"
#define IMAGES_TBL_FILE_PATH   "arr" DIR_SEPARATOR "images.tbl"
#define SFXDATA_TBL_FILE_PATH  "arr" DIR_SEPARATOR "sfxdata.tbl"

#define IMAGES_LST_FILE_PATH   "images.lst"
#define ISCRIPT_LST_FILE_PATH  "iscript.lst"

/* useful macros */
#define streq(a,b)     (!strcmp((a),(b)))

/* macro to quickly emulate tell on a mem mapped file */
#define mtell(mfp)   ((mfp)->pos)
/* macro to quickly get a byte out of the mem mapped file, EOF on eof */
#define mgetc(mfp)   ((mfp)->pos < (mfp)->size ? (mfp)->array[(mfp)->pos++] : EOF )
/* macro to get the file size */
#define msize(mfp)   ((mfp)->size)

extern MFILE *mopen(const char *pathname);
extern int mclose(MFILE *mfileptr);
extern int mseek(MFILE *mfp, long offset, int whence);
extern size_t mread(void *ptr, size_t size, register size_t nobj, MFILE *mfp);
extern size_t mwrite(const void *ptr, size_t size, register size_t nobj, MFILE *mfp);

extern MFILE *mcreat(long size);
extern int msave(char *file_name, MFILE *mfile);
extern void mresize(MFILE *mfile, long newsize);

extern void sc_register_prog_name(char *name);
extern char *sc_get_prog_name();
extern void sc_err_log(char *fmt, ...);
extern char *sc_get_err();
extern char *sc_get_err_prev();
extern char *sc_get_err_old() ;
extern void sc_err_fatal(char *fmt, ...);
extern void sc_err_warn(char *fmt, ...);
extern char *strip_path(const char *path);

#ifdef __DEBUG
extern void mprint_unread(MFILE *mfile);
#endif

#endif
