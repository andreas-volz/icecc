/*
  IceCC. These are shared routines
  Copyright (C) 2000-2001 Jeffrey Pang <jp@magnus99.dhs.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

/* $Id: scdef.c,v 1.15 2002/06/09 08:11:55 jp Exp $ */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "scdef.h"

static char program_name[32] = "";

/* error message buffers for sc functions that
   report errors. The last error will be saved in 
   the first one and the last 2 before that in the 
   next two. */
static char sc_error[256]      = "";
static char sc_error_prev[256] = "";
static char sc_error_old[256]  = "";

char *strip_path(const char *path) {
  static char buf[256];
  char *next, *last;
  buf[255] = '\0';

  strncpy(buf, path, 255);
  next = strtok(buf, DIR_SEPARATOR);

  do {
    last = next;
  } while ((next = strtok(NULL, DIR_SEPARATOR)) != NULL);

  return last;
}

void sc_register_prog_name(char *name) {
  program_name[31] = '\0';
  strncpy(program_name, strip_path(name), 31);
}

char *sc_get_prog_name() {
  return program_name;
}

/* return the last error that occured in an sc function */
char *sc_get_err() {
  return sc_error;
}

/* return the second to last errot that occurred */
char *sc_get_err_prev() {
  return sc_error_prev;
}

/* return the third last error that occurred */
char *sc_get_err_old() {
  return sc_error_old;
}

/* Log an error and continue -- this could
   buffer overflow, don't abuse it :P */
void sc_err_log(char *fmt, ...) {
  va_list ap;

  /* push back last two error messages */
  strcpy(sc_error_old, sc_error_prev);
  strcpy(sc_error_prev,sc_error);

  /* write new error message */
  va_start(ap, fmt);
  vsprintf(sc_error, fmt, ap);
  va_end(ap);
}

/* Report a fatal error and exit */
void sc_err_fatal(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "%s: error: ", program_name);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

/* Report a warning and continue */
void sc_err_warn(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "%s: warning: ", program_name);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  return;
}

/* Maps an entire binary file to memory for reading. Writing does not work.
   This function is useful to avoid disk IO if we're going to eventually
   read the entire file into memory somewhere anyway. (Warning: if the file
   is big, this may not be too helpful since the cpu will probably page most
   of it away on the disk anyway -- depending on how much memory you have
   and if you're using Window's messed up VM manager :) Returns a MFILE
   handle if successful, NULL on error */
MFILE *mopen(const char *pathname) {
  MFILE *mstream;
  FILE *stream;

  if ((stream = fopen(pathname, "rb")) == NULL)
    return NULL;

  mstream = malloc(sizeof(MFILE));

  if (fseek(stream, 0, SEEK_END) || (mstream->size = ftell(stream)) == -1L) {
    fclose(stream);
    free(mstream);
    return NULL;
  }

  mstream->array = malloc(mstream->size*sizeof(byte));
  fseek(stream, 0, SEEK_SET);

  if (fread(mstream->array, sizeof(byte), mstream->size, stream) != mstream->size) {
    fclose(stream);
    free(mstream->array);
    free(mstream);
    return NULL;
  }

#ifdef __DEBUG
  mstream->visited = calloc(mstream->size, sizeof(byte));
#endif
  
  fclose(stream);
  mstream->pos = 0;
  return mstream;
}

/* Returns a new mfile stream of size size in bytes. This is the only
   type of stream you should use mwrite to write to because then you
   will know the size and not try to write past the end. */
MFILE *mcreat(long size) {
  MFILE *mstream = malloc(sizeof(MFILE));
  mstream->array = malloc(sizeof(byte)*size);
  mstream->size = size;
  mstream->pos = 0;

#ifdef __DEBUG
  mstream->visited = calloc(mstream->size, sizeof(byte));
#endif

  return mstream;
}

/* Write out the mem mapped file to disk.
   -1 on error, 0 on success */
int msave(char *file_name, MFILE *mfile) {
  FILE *file = fopen(file_name, "wb");
  if (!file)
    return -1;

  if (fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    remove(file_name);
    return -1;
  }
  if (fwrite(mfile->array, sizeof(byte), mfile->size, file) != mfile->size) {
    fclose(file);
    remove(file_name);
    return -1;
  }

  fclose(file);
  return 0;
}

/* Resizes the mem mapped file to newsize in bytes. If the new size
   is greater than the old size, then the new data is undefined;
   if it is smaller then the old data is truncated. If the current
   file position is beyond the newsize, then it points to the end of
   the file */
void mresize(MFILE *mfile, long newsize) {
  mfile->array = realloc(mfile->array, newsize);

#ifdef __DEBUG
  mfile->visited = realloc(mfile->visited, newsize);
#endif

  mfile->size = newsize;
  if (mfile->pos > newsize)
    mfile->pos = newsize;
}

/* "Close" a memory mapped file. (Really just frees up the memory). 
   Returns 0. */
int mclose(MFILE *mfile) {
  free(mfile->array);

#ifdef __DEBUG
  free(mfile->visited);
#endif

  free(mfile);
  return 0;
}

int mseek(MFILE *mfp, long offset, int whence) {
#ifdef __DEBUG
  int i;
#endif
  switch(whence) {
  case SEEK_SET: 
    mfp->pos = offset;
    break;
  case SEEK_CUR: 
#ifdef __DEBUG
    /* mark spots which we "seek over" */
    for (i=0; i<abs(offset); i++)
      mfp->visited[mfp->pos+i] = 0x01;
#endif
    mfp->pos += offset;
    break;
  case SEEK_END:
    mfp->pos = mfp->size + offset;
    break;
  default:
    return -1L;
  }

  /* check to see that the new position is a valid index */
  if (mfp->pos > mfp->size || mfp->pos < 0)
    return -1L;
  return 0;
}

/* Read nobj objects of size size into the buffer at ptr from the mem-mapped file
   mfp. Right now this only works for sizes 1, 2, and 4. Designed to be as
   quick as possible. Returns the number of objects read. See inside if you're on
   a Big Endian machine like a Macintosh. There is a little difference this and
   regular fread, mread will not allow you to read past the end of the file; if
   you attempt to, it will only read in enough objects until it reaches the end
   and the file position marker will be placed at the point where doing one more
   read would have caused it to go past the end (i.e., if it detects its next read
   will go past the end, then it will stop and won't increment the file position
   marker any further. */
size_t mread(void *p, size_t size, register size_t nobj, MFILE *mfp) {
  register byte *ptr    = (byte *)p;
  register byte *array  = mfp->array + mfp->pos;
  register long offset  = mfp->pos;
  register long mfpsize = mfp->size;
  size_t   objsread = 0;

#ifdef __DEBUG
  byte *varray = mfp->visited + mfp->pos;
#endif

  switch(size) {
  case sizeof(byte):
    for (; nobj > 0; nobj--, ptr+=sizeof(byte), array+=sizeof(byte), offset+=sizeof(byte)) {
      if (offset+sizeof(byte) > mfpsize) 
	break;
#ifdef __DEBUG
      *(byte *)varray = 0x01;
      varray += sizeof(byte);
#endif
      *(byte *)ptr = *(byte *)array;
    }
    objsread = (offset - mfp->pos)/sizeof(byte);
    mfp->pos = offset;
    return objsread;
  case sizeof(uint16):
    for (; nobj > 0; nobj--, ptr+=sizeof(uint16), array+=sizeof(uint16), offset+=sizeof(uint16)) {
      /* The REVERSE_ENDIAN hacks are for Macintosh users. While PPCs are Big Endian
	 Blizzard decided some reason to keep the data files in Little Endian
	 even on Macs. :P Define REVERSE_ENDIAN before this function if you're on
	 a Mac */
#ifdef REVERSE_ENDIAN
      uint16 buf;
#endif
      if (offset+sizeof(uint16) > mfpsize)
	break;
#ifdef __DEBUG
      *(uint16 *)varray = 0x0101;
      varray += sizeof(uint16);
#endif
#ifndef REVERSE_ENDIAN
      *(uint16 *)ptr = *(uint16 *)array;
#else
      buf = *(uint16 *)array;
      *(uint16 *)ptr = (buf<<8)|(buf>>8);
#endif
    }
    objsread = (offset - mfp->pos)/sizeof(uint16);
    mfp->pos = offset;
    return objsread;
  case sizeof(uint32):
    for (; nobj > 0; nobj--, ptr+=sizeof(uint32), array+=sizeof(uint32), offset+=sizeof(uint32)) {
#ifdef REVERSE_ENDIAN
      uint32 buf;
#endif
      if (offset+sizeof(uint32) > mfpsize) 
	break;
#ifdef __DEBUG
      *(uint32 *)varray = 0x01010101;
      varray += sizeof(uint32);
#endif
#ifndef REVERSE_ENDIAN
      *(uint32 *)ptr = *(uint32 *)array;
#else
      buf = *(uint32 *)array;
      *(uint32 *)ptr = (buf<<24)|((buf<<8)&0x00FF0000)|((buf>>8)&0x0000FF00)|(buf>>24);
#endif
    }
    objsread = (offset - mfp->pos)/sizeof(uint32);
    mfp->pos = offset;
    return objsread;
  default:
    return 0;
  }
}

/* Write to a mem mapped file. Same warnings as mread. You may not write past the end
   of the mem mapped file size. If you need to, you should resize the file first */
size_t mwrite(const void *p, size_t size, register size_t nobj, MFILE *mfp) {
  register byte *ptr    = (byte *)p;
  register byte *array  = mfp->array + mfp->pos;
  register long offset  = mfp->pos;
  register long mfpsize = mfp->size;
  size_t   objsread = 0;

#ifdef __DEBUG
  byte *varray = mfp->visited + mfp->pos;
#endif

  switch(size) {
  case sizeof(byte):
    for (; nobj > 0; nobj--, ptr+=sizeof(byte), array+=sizeof(byte), offset+=sizeof(byte)) {
      if (offset+sizeof(byte) > mfpsize) 
	break;
#ifdef __DEBUG
      *(byte *)varray = 0x01;
      varray += sizeof(byte);
#endif
      *(byte *)array = *(byte *)ptr;
    }
    objsread = (offset - mfp->pos)/sizeof(byte);
    mfp->pos = offset;
    return objsread;
  case sizeof(uint16):
    for (; nobj > 0; nobj--, ptr+=sizeof(uint16), array+=sizeof(uint16), offset+=sizeof(uint16)) {
#ifdef REVERSE_ENDIAN
      uint16 buf;
#endif
      if (offset+sizeof(uint16) > mfpsize)
	break;
#ifdef __DEBUG
      *(uint16 *)varray = 0x0101;
      varray += sizeof(uint16);
#endif
#ifndef REVERSE_ENDIAN
      *(uint16 *)array = *(uint16 *)ptr;
#else
      buf = *(uint16 *)ptr;
      *(uint16 *)array = (buf<<8)|(buf>>8);
#endif
    }
    objsread = (offset - mfp->pos)/sizeof(uint16);
    mfp->pos = offset;
    return objsread;
  case sizeof(uint32):
    for (; nobj > 0; nobj--, ptr+=sizeof(uint32), array+=sizeof(uint32), offset+=sizeof(uint32)) {
#ifdef REVERSE_ENDIAN
      uint32 buf;
#endif
      if (offset+sizeof(uint32) > mfpsize) 
	break;
#ifdef __DEBUG
      *(uint32 *)varray = 0x01010101;
      varray += sizeof(uint32);
#endif
#ifndef REVERSE_ENDIAN
      *(uint32 *)array = *(uint32 *)ptr;
#else
      buf = *(uint32 *)ptr;
      *(uint32 *)array = (buf<<24)|((buf<<8)&0x00FF0000)|((buf>>8)&0x0000FF00)|(buf>>24);
#endif
    }
    objsread = (offset - mfp->pos)/sizeof(uint32);
    mfp->pos = offset;
    return objsread;
  default:
    return 0;
  }
}

#ifdef __DEBUG
/* Debugging routine to see which bytes in a mfile were
   never read or written to */
void mprint_unread(MFILE *mfile) {
  int i, last = -1;
  fprintf(stderr, "bytes not visited:");
  for (i=0; i<mfile->size; i++) {
    if (!mfile->visited[i] && last == -1)
      last = i;
    if (mfile->visited[i])
      last = -1;
    /* remember, visiting the last-element + 1 is legal */
    if (!mfile->visited[i] && last != -1 && mfile->visited[i+1]) {
      if (last == i)
	fprintf(stderr, " %04x", i);
      else
	fprintf(stderr, " %04x-%04x", last, i);
      last = -1;
    }
  }
  fprintf(stderr, "\n");
}
#endif

/* test memory allocation leaks and stuff */
#ifdef __DEBUG_MEMORY

#undef malloc
#undef calloc
#undef free

#define memtable_size 100000
static int memtable_init = 0;
static unsigned long memtable[memtable_size][3];

static unsigned long a, b, p;

static void init_memtable() {
    int i;

    for (i=0; i<memtable_size; i++) {
	memtable[i][0] = 0;
	memtable[i][2] = 0;
	memtable[i][3] = 0;
    }

    srand((unsigned)time(NULL));
    a = rand();
    b = rand();
    p = 4294967291UL;

    memtable_init = 1;
}

static int memtable_hash(unsigned long val) {
    return (int)( ( (a*val + b)%p ) % memtable_size );
}

static void insert_memtable(void *ptr, char *filename, int lineno) {
    unsigned long val = (unsigned long)ptr;
    int index = memtable_hash(val);

#ifdef MEMORY_WARN_ON_HASH_COLLISION
    if (memtable[index][0] != 0) {
	fprintf(stderr, "warning: overwriting memtable slot at %d\n", index);
	fprintf(stderr, "         previous was %s:%d\n",
		(char *)memtable[index][1], (int)memtable[index][2]);
    }
#endif

    memtable[index][0]++;
    memtable[index][1] = (unsigned long)filename;
    memtable[index][2] = (unsigned long)lineno;
}

static void remove_memtable(void *ptr, char *filename, int lineno) {
    unsigned long val = (unsigned long)ptr;
    int index = memtable_hash(val);

    if (memtable[index][0] <= 0) {
	fprintf(stderr, "ERROR! ptr %p,slot %d we think originally allocated at %s:%d\n",
		ptr, index, (char *)memtable[index][1], (int)memtable[index][2]);
	die("free called but memory already freed!", filename, lineno);
    }
    
    if (--memtable[index][0] == 0) {
	memtable[index][1] = 0;
	memtable[index][2] = 0;
    }
}

void *__my_malloc(int sz, char *filename, int lineno) {
    void *ptr;

    if (!memtable_init) init_memtable();

    if (!(sz >= 0)) die("size < 0", filename, lineno);
    if (sz == 0) {
	fprintf(stderr, "warning: size == 0 at %s:%d\n", filename, lineno);
	return NULL;
    }

    ptr = malloc(sz);

    insert_memtable(ptr, filename, lineno);

    return ptr;
}

void *__my_calloc(int num, int sz, char *filename, int lineno) {
    void *ptr;

    if (!memtable_init) init_memtable();

    if (!(num*sz >= 0)) die("size < 0", filename, lineno);
    if (num*sz == 0) {
	fprintf(stderr, "warning: size == 0 at %s:%d\n", filename, lineno);
	return NULL;
    }

    ptr = calloc(num,sz);

    insert_memtable(ptr, filename, lineno);

    return ptr;
}

void __my_free(void *ptr, char *filename, int lineno) {

    if (!memtable_init) init_memtable();

    if (ptr == NULL) return; /* freeing NULL is legal */

    remove_memtable(ptr, filename, lineno);

    free(ptr);
}

void print_memory_leaks() {
    int i;

    for (i=0; i<memtable_size; i++) {
	if (memtable[i][0] > 0) {
	    fprintf(stderr, "%d: %d leaks, last from %s:%d\n",
		    i, (int)memtable[i][0], (char *)memtable[i][1],
		    (int)memtable[i][2]);
	}
    }
}

#endif
