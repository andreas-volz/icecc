/*
  IceCC. This file implements handlers for *.Tbl files
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

/* $Id: tbl.c,v 1.8 2001/01/14 05:30:46 jp Exp $ */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "scdef.h"
#include "tbl.h"

/* This function does some error checking so you can pass in
   non-fully allocated Tbl structures (like the get_tbl_entries
   function does on cleanup), but you must make sure that
   non-allocated parts are set to NULL */
void tbl_free(Tbl *tbl_st) {
  int i;

  if (tbl_st == NULL) return;
  if (tbl_st->entries != NULL)
    /* there should not be anything in index 0,
       so we'll start from 1 */
    for (i=1; i<=tbl_st->size; ++i)
      if (tbl_st->entries[i] != NULL)
	free(tbl_st->entries[i]);
  free(tbl_st->entries);
  free(tbl_st);
}

/* Returns a newly malloced Tbl structure; tbl_file_name should
   be a *.Tbl file to open and read. NULL on error. */
Tbl *tbl_new(char *tbl_file_name) {
  MFILE  *tbl_file = mopen(tbl_file_name);
  addr   next, nextptr;
  Tbl    *tbl_st;
  int i;

  /* could not open file */
  if (tbl_file == NULL) {
    sc_err_log("tbl_new: could not open %s", tbl_file_name);
    return NULL;
  }

  tbl_st = calloc(1, sizeof(Tbl));
  if (mread(&(tbl_st->size), sizeof(uint16), 1, tbl_file) != 1) {
    sc_err_log("tbl_new: header read of %s TBL failed", tbl_file_name);
    /* clean up on error */
    mclose(tbl_file);
    tbl_free(tbl_st);
    return NULL;
  }

  /* allocate the string array; since Tbl files start
     indexing from 1, we'll leave the first entry of the
     array empty and add one more to the end */
  tbl_st->entries = calloc(tbl_st->size+1, sizeof(char *));

  nextptr = mtell(tbl_file);
  for(i=1; i <= tbl_st->size; ++i) {
    char buf[TBL_STRING_MAX_LEN];
    int count;

    /* find the next pointer */
    if (mseek(tbl_file, nextptr, SEEK_SET)) {
      sc_err_log("tbl_new: seek to pointer of %s TBL failed", tbl_file_name);
      mclose(tbl_file);
      tbl_free(tbl_st);
      return NULL;
    }
    /* read it */
    if (mread(&next, sizeof(addr), 1, tbl_file) != 1) {
      sc_err_log("tbl_new: read of pointer of %s TBL failed", tbl_file_name);
      mclose(tbl_file);
      tbl_free(tbl_st);
      return NULL;
    }
    nextptr = mtell(tbl_file); /* save location of next pointer */
    /* go to it */
    if (mseek(tbl_file, next, SEEK_SET)) {
      sc_err_log("tbl_new: seek to string of %s TBL failed", tbl_file_name);
      mclose(tbl_file);
      tbl_free(tbl_st);
      return NULL;
    }
    
    /* read the string */
    for (count=0; (buf[count++] = mgetc(tbl_file)) != 0;)
      ;
    /* alloc and copy it into our structure */
    tbl_st->entries[i] = malloc(count*sizeof(char)); /* count will include '\0' */
    strcpy(tbl_st->entries[i], buf); /* this should be safe */
  }

  mclose(tbl_file);
  return tbl_st;
}

/* Save the Tbl structure to the file file_name. Returns
   0 on success, -1 on error. Needs fix to work on Big Endian. */
int tbl_save(char *file_name, Tbl *tbl_st) {
  FILE   *tbl_file = fopen(file_name, "wb");
  addr   next;
  int i;

  /* could not open file */
  if (tbl_file == NULL) {
    sc_err_log("tbl_save: could not open %s for writing", file_name);
    return -1;
  }

  if (fwrite(&(tbl_st->size), sizeof(uint16), 1, tbl_file) != 1) {
    sc_err_log("tbl_save: header write to %s failed", file_name);
    fclose(tbl_file);
    remove(file_name);
    return -1;
  }

  /* first string goes past the #size headers + the 1 uint16
     that is we just wrote */
  next = (tbl_st->size+1)*sizeof(addr);
  for (i=1; i<=tbl_st->size; i++) {
    if (fwrite(&next, sizeof(uint16), 1, tbl_file) != 1) {
      sc_err_log("tbl_save: pointer write to %s failed", file_name);
      fclose(tbl_file);
      remove(file_name);
      return -1;
    }
    /* next pos to write is past the first string + null char */
    next = next+strlen(tbl_st->entries[i])+1;
  }
  for (i=1; i<=tbl_st->size; i++) {
    int len;
    /* write the string + null char */
    if (fwrite(tbl_st->entries[i], sizeof(char), 
	       len = strlen(tbl_st->entries[i])+1, tbl_file) != len) {
      sc_err_log("tbl_save: string write to %s failed", file_name);
      fclose(tbl_file);
      remove(file_name);
      return -1;
    }
  }

  fclose(tbl_file);
  return 0;
}
  

/* simple functions for working with Tbl structures. 
   GET_TBL_STRING returns a char * of the string, NULL on error
   GET_TBL_SIZE   returns the number of strngs in the Tbl.
   Remember that Tbl files start indexing from 1 and NOT 0,
   so the first Tbl string is GET_TBL_STRING(1). Getting the
   0th string will be null. Size is the number of strings in the
   Tbl, not the size of the array (it is 1+ since we start indexing
   at 1). Remember to use <= instead of < when iterating along a tbl_st */
char *tbl_get_string(Tbl *tbl_st, unsigned i) {
  return i > tbl_st->size? NULL : tbl_st->entries[i];
}

/* see previous definition */
unsigned tbl_get_size(Tbl *tbl_st) {
  return tbl_st->size;
}

/* Sets the string number i (remember, start indexing from 1)
   in the tbl_st to the new value. This function will free the old
   string and allocate space and copy the new one in. You can do
   whatever you want with the old one */
void tbl_set_string(Tbl *tbl_st, unsigned i, const char *newstr) {
  char *newstralloc = malloc((strlen(newstr)+1)*sizeof(char));

  strcpy(newstralloc,newstr);
  free(tbl_get_string(tbl_st, i));
  tbl_st->entries[i] = newstralloc;
}

/* Insert the new string newstr at index i in the Tbl structure. This
   function is not very efficient if you need to use it to add a lot of
   strings (you may want to go into the structure yourself). The new string
   is allocated its own space and copied so you can do whatever you want with
   the string you pass in. */
void tbl_insert_string(Tbl *tbl_st, unsigned i, const char *newstr) {
  /* +1 because we're indexing from 1 and +1 for the string we want to add */
  char **newarray = malloc((tbl_st->size+2)*sizeof(char *));
  char *newstralloc = malloc((strlen(newstr)+1)*sizeof(char));
  int j;

  strcpy(newstralloc,newstr);
  /* Tbl doesn't need a 0th entry */
  newarray[0] = NULL;
  for (j=1; j<=tbl_st->size+1; j++) {
    if (j<i)
      newarray[j] = tbl_st->entries[j];
    else if (j == i)
      newarray[j] = newstralloc;
    else
      newarray[j] = tbl_st->entries[j-1];
  }
  free(tbl_st->entries);
  tbl_st->entries = newarray;
  tbl_st->size++;
}
