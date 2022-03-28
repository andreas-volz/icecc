/*
  IceCC. These are the main iscript object interface functions.
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

/* $Id: iscript.c,v 1.21 2001/11/25 01:49:03 jp Exp $ */

/* #define __DEBUG2 */

#include <stdlib.h>
#include <string.h>
#include "scdef.h"
#include "iscript.h"
#include "iscript-instr.h"

#ifdef __DEBUG
#include <stdio.h>
#include "icecc-utils.h"
#endif

/* distance between what is a 'local' goto and a long goto */
#define LOCAL_JMP_DISTANCE 0x200

/* the iscript.bin has its header table at the start of the iscript,
   so we need to zero the space out for backward compatabilty, this
   is the size of that original table, not including the 0xFFFF0000
   marker at the end; size in bytes */
#define ORIG_STARCRAFT_HEADER_TABLE_SIZE 0x558

#define HEADER_TABLE_ADDR_PTR 0x0 /* the address in iscript.bin where to find
				     the pointer to the header table (for Broodwar's
				     iscript.bin) */

/* The array index corresponds to the header-type (first uint16 
   of a animation header) and the value corresponds to the number
   of animation entries that header-type has. 0 means the header
   type is unknown or unused. */
uint16 __header_types[] = { 2,  2,  4,  0,  0,  0,  0,  0, 
			    0,  0,  0,  0,  14, 14, 16, 16,
			    0,  0,  0,  0,  22, 22, 0,  24,
			    26, 28, 28, 28, 0,  0,  0,  0 };

/* IMPORTANT: do not use any of these global variables except in the Iscript
   constructor. Unfornunately, not thread safe. Will have to fix this. */
static HashTable *symtbl   = NULL; /* the symbol table with offset info */
static long      unique_id = 0;    /* guarantee unique id to each symtbl entry */
static HashTable *headers  = NULL; /* info for each animation header and its offsets */
static IsInstr **code    = NULL; /* the actual byte-code of the animation routines,
				      a huge array with them in order */
static long      codesize  = 0;    /* the size of the above array */

/* local helper functions */
static void err_free_headers(HashTable *headers);
static void err_free_symtbl(HashTable *st);
static void err_free_code(IsInstr **carray, long csize);
static int load_headers(MFILE *iscript, int version);
static int read_code(MFILE *iscript, IsHeader *hdrptr, uint16 animno);
static int init_bytecode_and_symtbl(MFILE *iscript);
static IsSymTblEnt *new_symtbl_entry(IsHeader *hdrptr, 
				      uint16 animno, 
				      IsInstr * jump_ptr, 
				      int type);
static void add_to_symtbl_entry(IsSymTblEnt* st_entry, 
				IsHeader *hdrptr, 
      				uint16 animno, 
				IsInstr *jump_ptr,
				int type);
static int write_bytecode(MFILE *mfile, IsInstr *bc);
static void free_bcenum_and_log(IsInstrEnum *bcenump, HashTable *symtbl_done);
static void __iscript_instr_visited_by(IsInstr *ip,
				       HashTable *visited, 
				       Queue *reached);
static void __iscript_symtblent_visited_by(IsSymTblEnt *st_entry,
					   HashTable *visited,
					   Queue *reached);
static int pointer_hash_fn(void *ptr);
static int pointer_eq_fn(void *p1, void *p2);
static IsSymTblEnt *__iscript_copy_symtblent(IsSymTblEnt *old,
					     IsHeader *header,
					     uint16 animno,
					     int type,
					     IsInstr *jump,
					     HashTable *visited,
					     ObjHashTable *visited_instrs);
static int anim_header_eq_fn(void *a, void *b);
static void __iscript_remove_symtblent(IsSymTblEnt *st, 
				       IsHeader *hp, 
				       HashTable *visited,
				       ObjHashTable *visitedinstrs,
				       ObjQueue *freeinstrs,
				       ObjQueue *disconnect);

/* debugging routines */
#ifdef __DEBUG
static HashTable *__hash = NULL;
static void print_bc(FILE *handle, int start, IsInstr *bc);
int super_debug_function();
#endif

/* Return a fully complete iscript object from a binary iscript.bin
   type file. Version is either STARCRAFT or BROODWAR. Creating an
   internal datastructure will work for both types, but saving may
   not work properly for starcraft iscript.bin files, only broodwar.
   Returns the fully constructed Iscript if OK, NULL on error
   (and an error message will probably be logged in sc_error) */
Iscript *iscript_new(char *file_name, int version) {
  MFILE *iscriptf = mopen(file_name);
  Iscript *iscript;

  if (iscriptf == NULL) {
    sc_err_log("iscript_new: could not open %s", file_name);
    return NULL;
  }

  /* reset the global variables before loading */
  symtbl    = NULL;
  unique_id = 0;  
  headers   = NULL;
  code      = NULL; 
  codesize  = 0;

  /* load from binary */
  if (load_headers(iscriptf, version) == -1) {
    mclose(iscriptf);
    return NULL;
  }
  if (init_bytecode_and_symtbl(iscriptf) == -1) {
    /* this was allocated by load_headers, so we have
       to delete it on error */
    err_free_headers(headers);
    mclose(iscriptf);
    return NULL;
  }

  /* assign values */
  iscript = malloc(sizeof(Iscript));
  iscript->headers = headers;
  iscript->symtbl  = symtbl;
  iscript->uniq_id = unique_id;
  iscript->version = version;

#ifdef __DEBUG
  mprint_unread(iscriptf);
#endif

  mclose(iscriptf);
  return iscript;
}

/* creates an empty iscript object and returns it
   (default version is BROODWAR, use the set version
   function to change) */
Iscript *iscript_new_empty() {
  Iscript *ip = malloc(sizeof(Iscript));

  ip->headers = hashtable_new(HEADERS_HASH_SIZE);
  ip->symtbl  = hashtable_new(SYMTBL_HASH_SIZE);
  ip->version = BROODWAR;
  ip->uniq_id = 0;

  return ip;
}

/* these are mini structs to help out the save to file
   function */
typedef struct idtoheader_pair {
  uint16 id;
  addr   offset;
} idtoheader_pair;

typedef struct symtbloffset_pair {
  long id;
  addr offset;
} symtbloffset_pair;

/* Helper function to write out each bytecode to the file,
   -1 on error */
static int write_bytecode(MFILE *mfile, IsInstr *bc) {
  int i;

#ifdef __DEBUG
  print_bc(stderr, mtell(mfile), bc);
#endif

  /* write the opcode */
  if (mwrite(&(bc->opcode), sizeof(byte), 1, mfile) != 1)
    return -1;
  /* write the arg */
  for (i=0; i < bc->numargs; i++) {
    if (isinstr_write_next_arg(mfile, bc->opcode, 
			  bc->args[i], i, bc->args[0]) == INSTR_WRITE_ERROR)
      return -1;
  }

  return 0;
}

/* We do the same things on every failure, so just keep it all up here */
#define SAVE_HANDLE_ERROR { \
  sc_err_log("iscript_save: save failed, produced file may have been larger than " \
          "maxnimum allowable iscript size: %d", MAX_ISCRIPT_FILE_SIZE); \
  mclose(mfile); \
  objqueue_freeall(idtoheaderq); \
  objqueue_freeall(relocq); \
  objqueue_free(todoq); \
  hashtable_free(symtbl_done); \
  hashtable_free(symtbl_id_offset_hash); \
  return -1; \
}

/* Save an Iscript object back to an isript binary file with name
   file_name. Returns 0 if OK, -1 on error. */
int iscript_save(char *file_name, Iscript *iscript) {
  static byte zero_buffer[] = { 0, 0 };
  static byte eof_buffer[]  = { 0xFF, 0xFF };

  /* pretend memory mapped file */
  MFILE *mfile = mcreat(MAX_ISCRIPT_FILE_SIZE);
  /* where we'll save our id->header pointers 
     for the table at the bottom */
  ObjQueue *idtoheaderq = objqueue_new();
  /* where we'll save our relocation offsets -- or the
     offsets where we have a reference to a symtbl entry
     but we don't yet know what address to write there 
     because we haven't written the actualy symtbl code
     yet; just like a relocation table */
  ObjQueue *relocq = objqueue_new();
  /* Todo queue of symtbl entries */
  ObjQueue *todoq = objqueue_new();
  HashTable *symtbl_done = hashtable_new(512);
  HashTable *symtbl_id_offset_hash = hashtable_new(512);
  HashEnum henum = hashenum_create(iscript->headers);
  IsHeader *hdrptr;
  addr header_table_offset, end_of_file;
  int i;

  if (mseek(mfile, 0, SEEK_SET))
    SAVE_HANDLE_ERROR;

  /* if the iscript is a original starcraft iscript
     then the header table is at the start of the file
     so we need to allocate room for it when we come
     back and write it later */
  if (iscript->version == STARCRAFT) {
    int numheaders = 0;
    for (;hashenum_next(&henum) != NULL; ++numheaders)
      if (mseek(mfile, 2*sizeof(uint16), SEEK_CUR))
	SAVE_HANDLE_ERROR;

    /* rebuild the henum because we still need it at
       its start */
    henum = hashenum_create(iscript->headers);
  } else {
    /* For Broodwar:
       zero out the first 558 bytes and add an EOF terminator
       at the end (0xFFFF0000), this is for compatibility for
       the original starcraft iscript.bin -- it's header table 
       started at the top and was exactly this long */  
    for (i=0; i < ORIG_STARCRAFT_HEADER_TABLE_SIZE/2; ++i)
      if (mwrite(zero_buffer, sizeof(uint16), 1, mfile) != 1)
	SAVE_HANDLE_ERROR;
  }
  /* 0xFFFF0000 terminator */
  if (mwrite(eof_buffer, sizeof(uint16), 1, mfile) != 1 ||
      mwrite(zero_buffer, sizeof(uint16), 1, mfile) != 1)
    SAVE_HANDLE_ERROR;

  while ((hdrptr = hashenum_next(&henum)) != NULL) {
    int noanims = iscript_numberof_anims(iscript, hdrptr->id);
    int i;
    idtoheader_pair *pair = malloc(sizeof(idtoheader_pair));

    /* If the offset we are currently at is not word aligned
       (word being 2-bytes), write a zero byte to align it.
       This isn't necessary, but it looks like all the header
       offsets in the iscript are word aligned, so we'll do that
       too (probably for efficiency -- though each IsAnim offset
       is not necessarily word aligned). The predicate in the if
       statement is just a quick bitwise AND way to determine
       if the current offset modulo 2 is non-zero. Little hack. :) */
    if (mtell(mfile)&0x1)
      if (mwrite(zero_buffer, sizeof(byte), 1, mfile) != 1)
	SAVE_HANDLE_ERROR;

    /* save a pointer to this header */    
    pair->id = hdrptr->id;
    pair->offset = mtell(mfile);
    objqueue_insert(idtoheaderq, pair);

    /* SCPE IsAnim header */
    if (mwrite("SCPE", sizeof(byte), 4, mfile) != 4)
      SAVE_HANDLE_ERROR;
    /* header type */
    if (mwrite(&(hdrptr->type), sizeof(uint16), 1, mfile) != 1)
      SAVE_HANDLE_ERROR;
    /* 2 byte spacer */
    if (mwrite(zero_buffer, sizeof(uint16), 1, mfile) != 1)
      SAVE_HANDLE_ERROR;
    for (i=0; i<noanims; i++) {
      /* we need to save this offset to fill in the pointer
	 so we can fill it in later when we know where it points to */
      if (hdrptr->st_entries[i] != NULL) {
	symtbloffset_pair *so_pair = malloc(sizeof(symtbloffset_pair));
	so_pair->id = hdrptr->st_entries[i]->id;
	so_pair->offset = mtell(mfile);
	objqueue_insert(relocq, so_pair);
	if (mseek(mfile, 2, SEEK_CUR))
	  SAVE_HANDLE_ERROR;
      } else {
	/* if there is no pointer to an animation (it is null)
	   then we can just write 0x0000 there */
	if (mwrite(zero_buffer, sizeof(uint16), 1, mfile) != 1)
	  SAVE_HANDLE_ERROR;
      }
    }

    /* write out the code for each IsAnim entry */
    for (i=0; i<noanims; i++) {
      IsInstrEnum bcenum;
      IsInstr *bc;
      if (hdrptr->st_entries[i] == NULL)
	continue;
      /* put this symtbl entry in our queue to start us
	 off */
      objqueue_insert(todoq, hdrptr->st_entries[i]);
      while (!objqueue_isempty(todoq)) {
	/* create the enumeration, rewind to the start, and go! :) */
	bcenum = isinstrenum_create_from_symtblent(objqueue_remove(todoq));
	isinstrenum_rewind(&bcenum);
	while((bc = isinstrenum_next(&bcenum)) != NULL) {
	  if (bc->st_entry != NULL) {
	    /* if we've already visited this instr, the rest is a loop, so we can stop */
	    if (hashtable_find(symtbl_done, bc->st_entry->id) != NULL)
	      break;
	    /* just a marker to show that we've already been here,
	       and a pointer to the label, so we can free it later. */
	    hashtable_insert(symtbl_done, bc->st_entry->id, bc->st_entry);
	    /* fill in the symtbl label's offset so we can fill in
	       relocation addresses later when we're all done -- yeah, the
	       (void *) cast is kinda a hack, but we don't really need anything
	       but the offset value, so it will do. This will break if we have
	       more than 2^16-1 symtbl entries... but I doubt that will happen. :)
	       Just make sure the symtbl ids don't get really high */
	    hashtable_insert(symtbl_id_offset_hash, (uint16)bc->st_entry->id, 
			     (void *)mtell(mfile));
	  }
	  /* write out the bytecode instruction to the file. */
	  if (write_bytecode(mfile, bc) == -1)
	    SAVE_HANDLE_ERROR;
	  /* if it is a jump instr, we need to save the code
	     which it jumps to so we can write it out later */
	  if (bc->jump != NULL) {
	    symtbloffset_pair *so_pair = malloc(sizeof(symtbloffset_pair));
	    objqueue_insert(todoq, bc->jump);
	    /* we also have to save the last argument's offset
	       in the relocation table because we don't really
	       know what address is supposed to go there yet */
	    so_pair->id = bc->jump->id;
	    /* last arg offset is current addr - sizeof 1 addr arg */
	    so_pair->offset = mtell(mfile)-sizeof(addr);
	    objqueue_insert(relocq, so_pair);
	  }
	}
      } /* while(!objqueue_isempty(todoq)) */
    } /* for (i=0; i<noanims; i++) */
  } /* while ((hdrptr = hashenum_next(idenum)) != NULL) */

  /* The starcraft iscript.bin needs to have its header table
     written at the top of the file, the broodwar file needs to
     have it written at the bottom and have a pointer saved
     to it written at the top -- there is actually also the header
     table at the bottom of the iscript.bin as well (the exact same
     table) at the offset 0x82e4... I'm not sure how that is found
     (I suspect it is hardcoded) since it follows an arbitary 0x101
     zero bytes and the offset value is not present in the file;
     hence, I'm skipping writing it, so starcraft iscript.bins
     created with this routine probably will not work. But this stuff
     is here just in case we want to try to make it work later. */
  if (iscript->version == STARCRAFT) {
    end_of_file = mtell(mfile);
    if (mseek(mfile, 0, SEEK_SET))
      SAVE_HANDLE_ERROR;
  } else
    /* save where we are so we can write it to the top of the iscript
       later */
    header_table_offset = mtell(mfile);
  
  /* now write out the iscript ID -> header offset table at the bottom
     of the iscript */
  while(!objqueue_isempty(idtoheaderq)) {
    idtoheader_pair *ih_pair = objqueue_remove(idtoheaderq);
    if (mwrite(&(ih_pair->id), sizeof(uint16), 1, mfile) != 1 ||
	mwrite(&(ih_pair->offset), sizeof(addr), 1, mfile) != 1)
      SAVE_HANDLE_ERROR;
    free(ih_pair); /* don't forget this :) */
  }
  
  /* 0xFFFF terminates the table and thus the iscript */
  if (mwrite(eof_buffer, sizeof(uint16), 1, mfile) != 1 ||
      /* write a zero buffer at the end, just in case */
      mwrite(zero_buffer, sizeof(uint16), 1, mfile) != 1)
    SAVE_HANDLE_ERROR;

  /* If we're a starcraft iscript.bin then we need to
     go back to the end of the file since we wrote the
     header table at the top, if we're broodwar, we're
     already there */
  if (iscript->version == STARCRAFT)
    if (mseek(mfile, end_of_file, SEEK_SET))
      SAVE_HANDLE_ERROR;

  /* truncate the file to this point, we don't need anymore
     beyond this */
  mresize(mfile, mtell(mfile));
  
  /* now go through the relocation q and go back to each
     offset to write where the symtbl labels actually point to */
  while(!objqueue_isempty(relocq)) {
    symtbloffset_pair *so_pair = objqueue_remove(relocq);
    addr offset;
    if (mseek(mfile, so_pair->offset, SEEK_SET))
      SAVE_HANDLE_ERROR;
    /* really icky casting (not all necessary, but is to avoid warnings... 
       I may clean this later, but it works for what its supposed to do */
    offset = (addr)(long)hashtable_find(symtbl_id_offset_hash, (uint16)so_pair->id);
    if (mwrite(&offset, sizeof(addr), 1, mfile) != 1)
      SAVE_HANDLE_ERROR;
    free(so_pair);
  }
  
  if (iscript->version != STARCRAFT) {
    /* finally, go back to the top and write the location of the
       table at the bottom */
    if (mseek(mfile, 0, SEEK_SET) ||
	mwrite(&header_table_offset, sizeof(uint16), 1, mfile) != 1)
      SAVE_HANDLE_ERROR;
  }

  /* mem mapped file is done. Just write it out and close it */
  if (msave(file_name, mfile) == -1)
    SAVE_HANDLE_ERROR;
  mclose(mfile);

  hashtable_free(symtbl_done);
  free(idtoheaderq);
  free(relocq);
  free(todoq);

  return 0;
}

/* free the entire Iscript structure */
void iscript_free(Iscript *iscript) {
  HashTable *symtbl_done = hashtable_new(512); /* keep track of labels we've past */
  int i;
  HashEnum henum = hashenum_create(iscript->headers);
  IsSymTblEnt *st_entry;
  IsHeader *hdrptr;

  /* recursively go through all the bytecode and free it,
     accumulating all the symtbl labels in our 'done' hash
     while we're at it */
  while ((hdrptr = hashenum_next(&henum)) != NULL) {
    int noanims = iscript_numberof_anims(iscript, hdrptr->id);
    for (i=0; i<noanims; i++) {
      IsInstrEnum bcenum;
      /* if the pointer is 0x0000 there is no IsAnim here or
	 if the pointer id is already in our done hash, we've
	 already visited this IsAnim, so we can continue */
      if (hdrptr->st_entries[i] == NULL ||
	  !hashtable_find(symtbl_done, hdrptr->st_entries[i]->id))
	continue;
      bcenum = isinstrenum_create(iscript, hdrptr->id, i);
      free_bcenum_and_log(&bcenum, symtbl_done);
    } 
  }
  
  /* now free all the symbol Tbl entries */
  henum = hashenum_create(iscript->symtbl);
  while ((st_entry = hashenum_next(&henum)) != NULL) {
    /* free the list of anims as well as the anims */
    objlist_freeall(st_entry->anims);
    /* free the list of jumps to this label, but not the jumps
       themselves since we already freed all the instrs */
    objlist_free(st_entry->jumps);
    /* free me */
    free(st_entry);
  }
  hashtable_free(iscript->symtbl);

  /* now free all the headers */
  henum = hashenum_create(iscript->headers);
  while ((hdrptr = hashenum_next(&henum)) != NULL) {
    /* already freed the symtbl entries, so only need to
       free the actual array of pointers to them */
    free(hdrptr->st_entries);
    free(hdrptr);
  }
  hashtable_free(iscript->headers);

  /* finally, free me */
  free(iscript);
  /* and free the temp hash */
  hashtable_free(symtbl_done);
}

/* helper function for freeing an Iscript */
static void free_bcenum_and_log(IsInstrEnum *bcenump, HashTable *symtbl_done) {
  IsInstr *bc;

  isinstrenum_rewind(bcenump);
  while ((bc = isinstrenum_next(bcenump)) != NULL) {
    /* see if there is a label pointing to us */
    if (bc->st_entry != NULL) {
      /* if we've already visited this instr, the rest is a loop, so we can stop */
      if (hashtable_find(symtbl_done, bc->st_entry->id) != NULL)
	return;
      /* just a marker to show that we've already been here,
	 and a pointer to the label, so we can free it later. */
      hashtable_insert(symtbl_done, bc->st_entry->id, bc->st_entry); 
    }
    /* if we're a jump and we haven't already freed the place we're jumping to, 
       recursively free the code we jump to */
    if (bc->jump != NULL && !hashtable_find(symtbl_done, bc->jump->id)) {
      IsInstrEnum jumpenum = isinstrenum_create_from_symtblent(bc->jump);
      free_bcenum_and_log(&jumpenum, symtbl_done);
    }
    /* now free me */
    if (bc->args != NULL)
      free(bc->args);
    free(bc);
  }
  /* loop and free the instrs after me */
}

/* set the version of the iscript to either STARCRAFT or
   BROODWAR... STARCRAFT does not currently save correctly
   (at least I don't think so). */
void iscript_set_version(Iscript *iscript, int version) {
  iscript->version = version;
}

/* Returns an enumeration object which you can use to iterate
   over iscript IDs. */
IsIdEnum isidenum_create(Iscript *iscript) {
  return hashenum_create(iscript->headers);
}

/* Returns the next iscript id in the iscid enumeration.
   Returns 0xFFFF if there are no more (note, make sure this
   isn't one of the ids you use! It shouldn't be). */
uint16 isidenum_next(IsIdEnum *idenum) {
  IsHeader *hdrptr;
  if ((hdrptr = hashenum_next(idenum)) == NULL)
    return (uint16)-1;
  return hdrptr->id;
}

/* Returns the number of animation scripts iscript ID id has;
   -1 if the id is invalid. */
int iscript_numberof_anims(Iscript *iscript, uint16 id) {
  IsHeader *hdrptr = hashtable_find(iscript->headers, id);
  if (hdrptr == NULL)
    return -1;
  return __header_types[hdrptr->type];
}

/* Returns true if the Iscript iscript has an animation script
   for the entry which uses Iscript ID iscript_id, in particular,
   for animation number animno */
int iscript_isvalid_anim(Iscript *iscript, 
	       uint16 iscript_id, 
	       int animno) 
{
  IsHeader *hdrptr;
  /* no such header */
  if (!(hdrptr = hashtable_find(iscript->headers, iscript_id)))
    return 0;
  /* animno is too big */
  if (animno >= __header_types[hdrptr->type])
    return 0;
  /* anim is valid, but there is no label assiciated
     with it */
  if (hdrptr->st_entries[animno] == NULL)
    return 0;
  return 1;
}

/* Finds the symtbl label associated with this ID in this
   iscript object; NULL if not found */
IsSymTblEnt *iscript_get_symtblent(Iscript *iscript, long id) {
  return hashtable_find(iscript->symtbl, id);
}

/* Finds the symtbl label by iscript id and animation number of
   that id. NULL if none */
IsSymTblEnt *iscript_get_symtblent_by_animno(Iscript *iscript, 
					     uint16 id, int animno) 
{
  IsHeader *hp = hashtable_find(iscript->headers, id);
  return hp? hp->st_entries[animno] : NULL;
}

/* Makes a enumeration object which is a stream of bytecode instructions.
   There is no memory allocated and the returned value is not a pointer.
   You can use the below functions to step through the instructions
   one at a time or go to a particular place in the stream. Note that
   you may start off in the middle of a bytecode stream and not at the start
   because the animation might start off at the middle and jump back to a
   place that is further back. You can use rewind to get back to the absolute
   starting point. Returns an empty enum if the args are not valid */
IsInstrEnum isinstrenum_create(Iscript *iscript,
			       uint16 iscript_id,
			       int animno)
{
  IsInstrEnum bcenum; /* init to zero just in case not valid */
  if (!iscript_isvalid_anim(iscript, iscript_id, animno)) {
    bcenum.next = NULL;
    return bcenum;
  }
  bcenum.next = ((IsHeader *)hashtable_find(iscript->headers, iscript_id))->st_entries[animno]->bc;
  return bcenum;
}

/* Returns the next bytecode instruction from this enumeration stream.
   Returns NULL if there are no more bytecodes to read (Warning: after
   returning NULL, the bcenum object is no longer valid and you can
   not get more objects out of it or even rewind it */
IsInstr *isinstrenum_next(IsInstrEnum *bcenum) {
  IsInstr *result = bcenum->next;
  if (result == NULL)
    return NULL;
  bcenum->next = bcenum->next->next;
  return result;
}
  
/* Rewinds the bytecode enumeration to the absolute start. Returns
   0 n success, -1 on error. You can not rewind a bytecode enumeration
   if you have already read past the end of it. */
int isinstrenum_rewind(IsInstrEnum *bcenum) {
  if (bcenum->next == NULL)
    return -1;
  while (bcenum->next->prev != NULL)
    bcenum->next = bcenum->next->prev;
  return 0;
}

/* Returns the symbol table 'label' that this instruction
   jumps to (if it is a jump instruction). You may call this
   on any instruction, but if it is not a jump, it will return
   NULL */
IsSymTblEnt *isinstr_get_jump(IsInstr *jumpinstr) {
  return jumpinstr->jump;
}

/* Returns the symbol table entry that points to this
   bytecode instruction. NULL if no label points to it. */
IsSymTblEnt *isinstr_get_label(IsInstr *bc) {
  return bc->st_entry;
}

/* returns the cannonical name of the instruction */
char *isinstr_name(IsInstr *instr) {
  return isinstr_get_name(instr->opcode);
}

/* Creates and returns a bytecode enumeration object from the 
   symbol table label st_entry. The stream pointer starts at the instruction
   which the label points to. Use rewind (above) to get to the very
   start of the stream. */
IsInstrEnum isinstrenum_create_from_symtblent(IsSymTblEnt *st_entry) {
  IsInstrEnum bcenum;
  bcenum.next = st_entry->bc;
  return bcenum;
}

/* This function returns an list of IsAnim structures which hold information
   about this particular symtbl label. In particular, each IsAnim structure
   in the list tells you which headers can 'get to' this particular label
   (either through a direct header reference or through a series of jumps)
   and how they got there (HEADER for a header ref, LOCALJMP for a jump that
   was close in proximity or LONGJMP for a jump that was from far away). See
   the IsAnim struction in iscript.h for more info. WARNING: do not alter
   the list or any of its contents. (Also note that the information in
   this list is not 100% accurate -- you should only expect it to contain
   all the headers which point directly to this label and *possibly* some
   jumps which point to this label (but they may not be listed); mainly used
   for label-name-choosing by the dissassembler) */
ObjList *symtblent_get_anim_list(IsSymTblEnt *st_entry) {
  return st_entry->anims;
}

/* Returns a linked list of all the header id's that have animations
   which can reach this instruction. You are responsible for freeing
   the list (with list_free()); NULL on error */
List *isinstr_reached_by(IsInstr *ip) {
  HashTable *visited = hashtable_new(64); /* symtbl entries go in here */
  Queue *reached = queue_new(); /* header ids go here */
  List *result = list_new();
 
  __iscript_instr_visited_by(ip, visited, reached);

  hashtable_free(visited);
  while (!queue_isempty(reached)) {
    long id = queue_remove(reached);
    if (!list_find(result, id))
	result = list_insert(result, id);
  }
  queue_free(reached);
  return result;
}

/* helper */
static void __iscript_instr_visited_by(IsInstr *ip,
				       HashTable *visited, 
				       Queue *reached)
{
  for (; ip != NULL; ip = ip->prev) {
    if (ip->st_entry != NULL && 
	!hashtable_find(visited, ip->st_entry->id))
      {
	hashtable_insert(visited, ip->st_entry->id, (void *)1);
	__iscript_symtblent_visited_by(ip->st_entry, visited, reached);
      }
  }
}

/* helper, mutually recursive with the above */
static void __iscript_symtblent_visited_by(IsSymTblEnt *st_entry,
					   HashTable *visited,
					   Queue *reached)
{
  ObjListEnum enumeration = objlistenum_create(st_entry->anims);
  IsAnim *anim;
  IsInstr *instr;

  /* for each header that points to us, log it */
  while ((anim = objlistenum_next(&enumeration)) != NULL) {
    if (anim->type == HEADER)
      queue_insert(reached, anim->header->id);
  }

  enumeration = objlistenum_create(st_entry->jumps);

  /* recursively look at the instrs which jump to us */
  while ((instr = objlistenum_next(&enumeration)) != NULL)
    __iscript_instr_visited_by(instr, visited, reached);
}
 
/* hashtable functions */
static int pointer_hash_fn(void *ptr) {
  return abs((int)ptr);
}

static int pointer_eq_fn(void *p1, void *p2) {
  return p1 == p2;
}

/* returns a new Iscript that only contains the necessary
   stuff for the header with id; e.g., useful if you only want
   to extract a single header's animations (also removes
   "sharing" of any routines between two headers) */
Iscript *iscript_extract_header(Iscript *iscript, uint16 id) {
  Iscript      *iscript_new;
  IsHeader     *hp, *new_hp;
  HashTable    *visited;        /* old id -> new symtbl ptr */
  ObjHashTable *visited_instrs; /* old instr ptr -> new instr ptr */
  HashEnum     enumeration;
  HashTable    *headers_new, *symtbl_new;
  IsSymTblEnt  *st_entry;
  int          i;

  if ((hp = hashtable_find(iscript->headers, id)) == NULL)
    return NULL;
  visited = hashtable_new(8);
  visited_instrs = objhashtable_new(128, pointer_hash_fn, pointer_eq_fn);
  new_hp = calloc(1, sizeof(IsHeader));
  
  new_hp->id = hp->id;
  new_hp->type = hp->type;
  new_hp->st_entries = 
    malloc(__header_types[new_hp->type]*sizeof(IsSymTblEnt *));

  /* need to init this -- global var, be careful */
  unique_id = 0;

  for (i=0; i<__header_types[new_hp->type]; i++) {
    if (hp->st_entries[i] == NULL) {
      new_hp->st_entries[i] = NULL;
      continue;
    }
    new_hp->st_entries[i] = 
      __iscript_copy_symtblent(hp->st_entries[i], 
			       new_hp,
			       i,
			       HEADER,
			       NULL,
			       visited,
			       visited_instrs);
  }

  headers_new = hashtable_new(HEADERS_HASH_SIZE);
  symtbl_new  = hashtable_new(SYMTBL_HASH_SIZE);

  hashtable_insert(headers_new, id, new_hp);

  enumeration = hashenum_create(visited);
  while ((st_entry = hashenum_next(&enumeration)) != NULL)
    hashtable_insert(symtbl_new, st_entry->id, st_entry);

  hashtable_free(visited);
  objhashtable_free(visited_instrs);

  iscript_new = malloc(sizeof(Iscript));
  iscript_new->headers = headers_new;
  iscript_new->symtbl  = symtbl_new;
  iscript_new->uniq_id = unique_id;

  return iscript_new;
}

/* args: old st entry to copy, pointer to the new header, the animno
   of the header where we arrived from, the type of jump that brought
   us here (HEADER, LOCALJMP, LONGJMP), a jump if a instr jump, else
   NULL, hash to keep track of st entries we've visited, hash to keep
   track of all the instructions we've visited;

   This function is very messy. Sorry. I kinda winged it and it got a bit
   out of control. :) */
static IsSymTblEnt *__iscript_copy_symtblent(IsSymTblEnt *old,
					     IsHeader *header,
					     uint16 animno,
					     int type,
					     IsInstr *jump,
					     HashTable *visited,
					     ObjHashTable *visited_instrs)
{
  IsSymTblEnt *new;
  IsAnim *anim;
  IsInstrEnum enumeration;
  IsInstr *instr, *instr_new;
  IsInstr *prev;
  /*int started;*/

  if (old == NULL)
    return NULL;
  /* if we already made this symtbl ent copy then just
     add the new anim back pointer to it (and possibly
     a jump) and return it */
  if ((new = hashtable_find(visited, old->id)) != NULL) {
    anim = malloc(sizeof(IsAnim));
    anim->header = header;
    anim->animno = animno;
    anim->type = type;
    objlist_insert(new->anims, anim);
    if (jump != NULL && !objlist_find(new->jumps, jump, pointer_eq_fn))
      objlist_insert(new->jumps, jump);
    return new;
  }

  /* give it a new id */
  new = calloc(1, sizeof(IsSymTblEnt));
  new->id = unique_id++; /* this must be initialize before this */

  /* mark that we've been here */
  hashtable_insert(visited, old->id, new);
  
  /* make a new anim back pointer for it */
  anim = malloc(sizeof(IsAnim));
  anim->header = header;
  anim->animno = animno;
  anim->type = type;
  new->anims = objlist_new();
  objlist_insert(new->anims, anim);
  new->anims = objlist_new();
  objlist_insert(new->anims, anim);

  /* copy the jump if necessary */
  new->jumps = objlist_new();
  if (jump != NULL) 
    objlist_insert(new->jumps, jump);

  /* see if we've already wrote the instructions
     to where we point to; in that case, just hook us up */
  if ((instr = objhashtable_find(visited_instrs, old->bc)) != NULL) {
    new->bc = instr;
    instr->st_entry = new;
    return new;
  }

  enumeration = isinstrenum_create_from_symtblent(old);
  /*isinstrenum_rewind(&enumeration);*/
  /*started = 0;*/
  prev = NULL;
  while ((instr = isinstrenum_next(&enumeration)) != NULL) {
    /* we may not need the entire instr chain 
       -- changed it so we don't rewind... shouldn't need this
       anymore */
    /*if (!started) {
      List *reached = isinstr_reached_by(instr);
      if (!list_find(reached, header->id)) {
	list_free(reached);
	continue;
      }
      list_free(reached);
      }
      started = 1;*/

    /* see if the instruction was already written (possible
       if there was a cond jump before us that jumped to
       somewhere infront of us) */
    if ((instr_new = objhashtable_find(visited_instrs, instr)) != NULL) {
      /* hook us up to our label */
      if (instr->st_entry == old) {
	new->bc = instr_new;
	instr_new->st_entry = new;
      }
      /* link us up to the guy before us */
      if (prev == NULL)
	instr_new->prev = NULL;
      else {
	instr_new->prev = prev;
	prev->next = instr_new;
      }
      break;
      /* don't need to continue since we assume that the rest
	 is already written, we just needed to connect ourselves
	 to it */
    }

    instr_new = calloc(1, sizeof(IsInstr));
    /* register ourselves so we don't loop back here */
    objhashtable_insert(visited_instrs, instr, instr_new);

    /* hook us up to our label */
    if (instr->st_entry == old) {
      new->bc = instr_new;
      instr_new->st_entry = new;
    }
    /* link us up to the guy before us */
    if (prev == NULL)
      instr_new->prev = NULL;
    else {
      instr_new->prev = prev;
      prev->next = instr_new;
    }
    prev  = instr_new;
    
    /* copy our arguments */
    instr_new->opcode = instr->opcode;
    instr_new->numargs = instr->numargs;
    instr_new->args = malloc(instr_new->numargs*sizeof(uint16));
    memcpy(instr_new->args, instr->args, instr_new->numargs*sizeof(uint16));
    if (instr->jump != NULL)
      instr_new->jump = __iscript_copy_symtblent(instr->jump,
						 header,
						 animno,
						 LOCALJMP, /* all jumps are local
							      since none are shared 
							      -- only 1 header */
						 instr_new,
						 visited,
						 visited_instrs);
  }

  return new;
}

/* determines if a and b's header pointers are the same 
   header; helper function for removing list nodes */
static int anim_header_eq_fn(void *a, void *b) {
  return ((IsAnim *)a)->header->id == ((IsAnim *)b)->header->id;
}

void iscript_remove_header(Iscript *iscript, uint16 id) {
  IsHeader *hp = hashtable_find(iscript->headers, id);
  HashTable *visited = hashtable_new(8);
  ObjHashTable *visitedinstrs = objhashtable_new(128, pointer_hash_fn, pointer_eq_fn);
  ObjQueue *freeinstrs = objqueue_new();
  ObjQueue *disconnect = objqueue_new();
  HashEnum enumeration;
  IsSymTblEnt *st;
  IsInstr *ip;
  int i;

  if (hp == NULL)
    return;

  for (i=0; i<__header_types[hp->type]; i++) {
    if (hp->st_entries[i] == NULL)
      continue;
    __iscript_remove_symtblent(hp->st_entries[i], hp, 
			       visited, visitedinstrs, freeinstrs, disconnect);
  }
  objhashtable_free(visitedinstrs);

  /* free all the instructions we accumulated */
  while (!objqueue_isempty(freeinstrs)) {
    ip = objqueue_remove(freeinstrs);
    /* if we're a jump, we have to remove the backpointers from the
       label we jump to */
    if (ip->jump != NULL)
      while (objlist_remove(ip->jump->jumps, ip, pointer_eq_fn) != NULL)
	/* do nothing, since the pointer is back to us */ ;
    /* safe to free me */
    free(ip->args);
    /* disconnect me */
    if (ip->next != NULL)
      ip->next->prev = NULL;
    if (ip->prev != NULL)
      ip->prev->next = NULL;
    free(ip);
  }
  objqueue_free(freeinstrs);

  /* disconnect all the instrs to labels we are going to
     free */
  while (!objqueue_isempty(disconnect))
    ((IsInstr *)objqueue_remove(disconnect))->st_entry = NULL;
  objqueue_free(disconnect);

  /* remove all the backpointers to the header in the
     symtbl... this is not ideal, since we have to go
     through the entire hashtable, but to be cleaner, we
     really have to fix the bigger mess and eliminate
     local/long jump distinctions and their inclusion in
     symtbl anims backpointer list... save problem for 
     another day */
  enumeration = hashenum_create(iscript->symtbl);
  while ((st = hashenum_next(&enumeration)) != NULL) {
    IsAnim test;
    IsAnim *anim;
    test.header = hp;

    while ((anim = objlist_remove(st->anims, &test, 
				  anim_header_eq_fn)) != NULL)
      free(anim);
  }

  /* remove all the symtbl's we visited and only belong
     to this header */
  enumeration = hashenum_create(visited);
  while ((st = hashenum_next(&enumeration)) != NULL) {
    /* only remove this guy if it points to nothing,
       signifying that we intended to remove it */
    if (st->bc == NULL) {

#ifdef __DEBUG2
      /* do some sanity checks */
      {
	ObjListEnum enum2 = objlistenum_create(st->anims);
	IsAnim *hp2;
	
	if (!objlist_isempty(st->anims)) {
	  fprintf(stderr, "*** ERROR: st id %d ids: ",
		  (int)st->id);
	  while ((hp2 = objlistenum_next(&enum2)) != NULL) {
	    fprintf(stderr, "%d ", (int)hp2->header->id);
	  }
	  fprintf(stderr, "\n");
	}
      }
#endif

      hashtable_remove(iscript->symtbl, st->id);
      objlist_free(st->anims);
      objlist_free(st->jumps);
      free(st);
    }
  }
  hashtable_free(visited);

  hashtable_remove(iscript->headers, id);
  free(hp->st_entries);
  free(hp);
}

static void __iscript_remove_symtblent(IsSymTblEnt *st, 
				       IsHeader *hp, 
				       HashTable *visited,
				       ObjHashTable *visitedinstrs,
				       ObjQueue *freeinstrs,
				       ObjQueue *disconnect) 
{
  List *reached;
  IsInstrEnum enumeration;
  IsInstr *instr;

  /* already been here */
  if (hashtable_find(visited, st->id))
    return;
  hashtable_insert(visited, st->id, st);

  /* other headers still use us, so keep us */
  reached = isinstr_reached_by(st->bc);

#ifdef __DEBUG2
  {
    List *tmp;
    fprintf(stderr, "*** symtbl %d reached by ids: ", (int)st->id);
    tmp = reached;
    while (tmp != NULL) {
      fprintf(stderr, "%d ", (int)tmp->head);
      tmp = tmp->tail;
    }
    fprintf(stderr, "\n");
  }
#endif

  if (list_length(reached) > 1) {
    list_free(reached);
    return;
  }
  list_free(reached);

  objqueue_insert(disconnect, st->bc);

  enumeration = isinstrenum_create_from_symtblent(st);
  while ((instr = isinstrenum_next(&enumeration)) != NULL) {
    /* make sure we haven't already been here */
    if (objhashtable_find(visitedinstrs, instr))
	break;
    objhashtable_insert(visitedinstrs, instr, (void *)1);
    if (instr->st_entry != NULL)
      /* recursively remove the symtbl's that point to us, OK if its
	 already one we've been to, since it will be in our visited hash */
      __iscript_remove_symtblent(instr->st_entry, hp, 
				 visited, visitedinstrs, freeinstrs, disconnect);

    /* see if we can remove this guy without disturbing any other headers,
       if not, then we can stop, since if others can reach us, they can surely
       reach all the instructions after us */
    reached = isinstr_reached_by(instr);
    if (list_length(reached) > 1) {
      list_free(reached);
      break;
    }
    list_free(reached);

    /* recursively remove the symtbls we jump to */
    if (instr->jump != NULL)
      __iscript_remove_symtblent(instr->jump, hp, 
				 visited, visitedinstrs, freeinstrs, disconnect);

    /* save me to free later */
    objqueue_insert(freeinstrs, instr);
  }

  st->bc = NULL;
  
  /* don't free the symtblent just yet, since we may still have instrutions
     which point to it later. we can tell which ones to free in our
     visited hashtable by seeing if the bc pointer is NULL */
}

/* Merges the source iscript with dest by combining all their
   headers and animations. Source becomes invalid after the merge
   (you don't have to free it or anything, just don't use it again)
   and it returns dest */
Iscript *iscript_merge(Iscript *dest, Iscript *source) {
  HashEnum enumeration;
  IsSymTblEnt *st;
  IsHeader *hp;

  /* Put each header from the old iscript into the new one,
     removing one with an identical iscript ID if necessary */
  enumeration = hashenum_create(source->headers);
  while ((hp = hashenum_next(&enumeration)) != NULL) {
    if (hashtable_find(dest->headers, hp->id))
      iscript_remove_header(dest, hp->id);
    hashtable_insert(dest->headers, hp->id, hp);
  }

  enumeration = hashenum_create(source->symtbl);
  /* give each symtbl in the old iscript a new unique id
     with relation to the symtbl in the new iscript, and
     merge the symtbls */
  while ((st = hashenum_next(&enumeration)) != NULL) {
    st->id = dest->uniq_id++;
    hashtable_insert(dest->symtbl, st->id, st);
  }

  /* invalidate the old iscript */
  hashtable_free(source->headers);
  hashtable_free(source->symtbl);
  free(source);

  return dest;
}

/* separates all the headers so that no two
   share any instructions and returns the new
   iscript. The argument passed in is unaffected. */
Iscript *iscript_separate_headers(Iscript *ip) {
  Iscript *ip_single;
  Iscript *ip_rest = iscript_new_empty();
  IsIdEnum enumeration = isidenum_create(ip);
  uint16 id;

  ip_rest->version = ip->version;

  while ((id = isidenum_next(&enumeration)) != (uint16)-1) {
    ip_single = iscript_extract_header(ip, id);
    ip_rest = iscript_merge(ip_rest, ip_single);
  }

  return ip_rest;
}
  

/* *** Constructor helper functions *** */

/* Frees the headers structure on error. Will free all the stuff
   that is non-NULL */
static void err_free_headers(HashTable *headers) {
  HashEnum henum;
  IsHeader *hdrptr;
  
  if (headers == NULL)
    return;
  else
    henum = hashenum_create(headers);

  while ((hdrptr = hashenum_next(&henum)) != NULL) {
    if (hdrptr->offsets != NULL)
      free(hdrptr->offsets);
    if (hdrptr->st_entries != NULL)
      free(hdrptr->st_entries);
    free(hdrptr);
  }

  hashtable_free(headers);
}

/* Initializes headers from file iscript. headers should not be
   defined before calling this. The file handle should already
   be open before calling this. Returns 0 if OK; -1 on error */
static int load_headers(MFILE *iscript, int version) {
  uint16      id;
  addr        offset;
  IsHeader *hdrptr;
  addr        offsets[MAX_ANIM_ENTRIES];
  HashEnum    enumeration;
  
  headers = hashtable_new(HEADERS_HASH_SIZE);
  
  /* The original Starcraft iscript.bin starts at the top */
  if (version == STARCRAFT)
    offset = 0;
  else {
    /* table pointer */
    if (mseek(iscript, HEADER_TABLE_ADDR_PTR, SEEK_SET)) {
      sc_err_log("load_headers: could not seek to table pointer");
      err_free_headers(headers);
      return -1;
    }
    if (mread(&offset, sizeof(offset), 1, iscript) != 1) {
      sc_err_log("load_headers: error reading table pointer");
      err_free_headers(headers);
      return -1;
    }
  }

  /* goto table */
  if (mseek(iscript, offset, SEEK_SET)) {
    sc_err_log("load_headers: could not seek to header table");
    err_free_headers(headers);
    return -1;
  }
  
  /* read each entry into our hashtable -- keyed by the ID */
  id = NULL_UINT16;
  while (id != EOF_UINT16) {
    if (mread(&id, sizeof(id), 1, iscript) != 1) {
      sc_err_log("load_headers: error reading table entry at 0x%04x", 
	      (unsigned)mtell(iscript));
      err_free_headers(headers);
      return -1;
    }
    if (id == EOF_UINT16)
      break;
    if (mread(&offset, sizeof(offset), 1, iscript) != 1) {
      sc_err_log("load_headers: error reading table entry at 0x%04x", 
	      (unsigned)mtell(iscript));
      err_free_headers(headers);
      return -1;
    }
    if (offset == EOF_UINT16) {
      sc_err_log("load_headers: read EOF in table before expected at 0x%04x", 
		(unsigned)mtell(iscript));
      err_free_headers(headers);
      return -1;
    }
    hdrptr = calloc(1, sizeof(IsHeader));
    hdrptr->id = id;
    hdrptr->offset = offset;
    
    hashtable_insert(headers, id, hdrptr);
  }

  /* now go through each header's entries in the iscript and record them */
  enumeration = hashenum_create(headers);
  while ((hdrptr = (IsHeader *)hashenum_next(&enumeration)) != NULL) {
    int i;
    if (mseek(iscript, hdrptr->offset, SEEK_SET) ||
	mseek(iscript, 4, SEEK_CUR)) /* offset + 'SCPE' */
      {
	sc_err_log("load_headers: could not seek to header offset at 0x%04x", 
	      (unsigned)hdrptr->offset);
	err_free_headers(headers);
	return -1;
      }
    if (mread(&id, sizeof(uint16), 1, iscript) != 1)
      {
	sc_err_log("load_headers: error reading header type at 0x%04x", 
		(unsigned)mtell(iscript));
	err_free_headers(headers);
	return -1;
      }
    hdrptr->type = id;
    if (mseek(iscript, 2, SEEK_CUR)) /* 2 byte padding */
      {
	sc_err_log("load_headers: could not seek past header padding at 0x%04x", 
		(unsigned)hdrptr->offset);
	err_free_headers(headers);
	return -1;
      }
    if (mread(offsets, sizeof(addr), __header_types[id], iscript) != __header_types[id])
      {
	sc_err_log("load_headers: error reading header entries at 0x%04x", 
		(unsigned)mtell(iscript));
	err_free_headers(headers);
	return -1;
      }

    /* allocate space for this header's animation pointers */
    hdrptr->offsets    = malloc(__header_types[id]*sizeof(addr));
    /* these will be filled in later */
    hdrptr->st_entries = calloc(__header_types[id], sizeof(IsSymTblEnt *));
    for (i=0; i < __header_types[id]; ++i)
      hdrptr->offsets[i] = offsets[i];
  }

  return 0;
}

/* This function assumes iscript has its file pointer set to a point
   where byte-code starts. It reads following conditional and unconditional
   jumps until there is no more to read. It fills in the code and symtbl
   structures while it is at it. Assumes both code and symtbl are initialized. 
   Returns 0 if successful, -1 on error. On either success or error, the caller
   is responsible for deallocating any bytecodes that read_code may have allocated
   (they should be in the code array). The bytecode will have an arg array allocated
   if it is not NULL */
static int read_code(MFILE *iscript, IsHeader *hdrptr, uint16 animno) {
  Queue *todo = queue_new(); /* addrs to more code goes in here */
  uint16 args[MAX_INSTR_ARGS];
  int keepgoing = 1;
  uint16 blockbegin = mtell(iscript);

  /* keep reading code until we hit a dead end and our todo queue
     (which fills up with conditional jumps) is empty. Or until we
     hit some kind of loop and we see code we've already read before */
  while (keepgoing) {
    IsInstr *bc;
    IsSymTblEnt *st_entry;
    byte opcode;
    addr offset;
    int start, argval, i, numargs = 0;

    /* see where we are */
    if ((start = mtell(iscript)) == -1L) {
      sc_err_log("read_code: mtell error on iscript before reading");
      queue_free(todo);
      return -1;
    }

    /* this means we already read this code before, don't need
       to do it again */
    if (code[start] != NULL) {
      if (queue_isempty(todo))
	break;
      else {
	/* pluck off the next address from the todo queue and restart */
	if (mseek(iscript, offset = queue_remove(todo), SEEK_SET)) {
	  sc_err_log("read_code: could not (condjmp) seek to %04x",
		  (unsigned)offset);
	  queue_free(todo);
	  return -1; 
	}
	continue;
      }
    }

    /* allocate the byte code entry and see if it has a corresponding
       symtbl entry */
    bc = calloc(1, sizeof(IsInstr));
    code[start] = bc;
    /*bc->st_entry = hashtable_find(symtbl, start); do this later */
    
    /* get the opcode */
    if(mread(&opcode, sizeof(byte), 1, iscript) != 1) {
      sc_err_log("read_code: error reading opcode at 0x%08x (block start: 0x%08x)", 
		 (unsigned)start, (unsigned)blockbegin);
      queue_free(todo);
      return -1;
    }
    if (opcode > MAX_OPCODE) {
      sc_err_log("read_code: unknown opcode 0x%02x read at 0x%04x (block start: 0x%08x)", 
	      (unsigned)opcode, (unsigned)start, (unsigned)blockbegin);
      queue_free(todo);
      return -1;
    }
    bc->opcode = opcode;

    /* read in the args */
    while((argval = isinstr_get_next_arg(iscript, opcode, &args[numargs], 
				    numargs, args[0])) != INSTR_READ_DONE) 
      {
	if (argval == INSTR_READ_ERROR) {
	  queue_free(todo);
	  return -1;
	}
	++numargs;
      }

    /* copy the args to the byte code structure */
    bc->numargs = numargs;
    if (numargs > 0) {
      bc->args = malloc(sizeof(uint16)*numargs);
      for (i=0; i<numargs; i++)
	bc->args[i] = args[i];
    } else
      bc->args = NULL;

    /* if the instr was a jump, the offset was the last
       argument. save it here */
    offset = args[numargs-1];

#ifdef __DEBUG
    bc->offset = start; /* save the instr offset if we're debugging */
#endif
    
    /* decide what to do depending on what type of instr this is */
    switch(isinstr_get_type(opcode)) {
    case INSTR_NORMAL:
      /* this is a placeholder to tell us to fill in the 'next'
	 pointer later */
      bc->next = (IsInstr *)-1;
      break;
    case INSTR_JMP:
      bc->next = NULL;
      
      /* jump to new location in iscipt */
      if (mseek(iscript, offset, SEEK_SET)) {
	sc_err_log("read_code: could not (jmp) seek to %04x",
		(unsigned)offset);
	queue_free(todo);
	return -1;
      }
      /* add a new symtbl entry for this offset 
         the addr was read in as the last argval */
      if (!(st_entry = hashtable_find(symtbl, offset)))
	hashtable_insert(symtbl,offset,
		   new_symtbl_entry(hdrptr, animno, bc,
				    abs(offset-start)>LOCAL_JMP_DISTANCE?
				    LONGJMP:LOCALJMP));
      else
	add_to_symtbl_entry(st_entry, hdrptr, animno, bc,
			    abs(offset-start)>LOCAL_JMP_DISTANCE?
			    LONGJMP:LOCALJMP);
      break;
    case INSTR_COND_JMP:
      bc->next = (IsInstr *)-1;

      /* we'll get back to this later */
      queue_insert(todo, offset);
      /* add a new symtbl entry for this offset, so we can find it later 
         the addr was read in as the last argval */
      if (!(st_entry = hashtable_find(symtbl, offset)))
	hashtable_insert(symtbl,offset,
		   new_symtbl_entry(hdrptr, i, bc,
				    abs(offset-start)>LOCAL_JMP_DISTANCE?
				    LONGJMP:LOCALJMP));
      else
	add_to_symtbl_entry(st_entry, hdrptr, i, bc,
			    abs(offset-start)>LOCAL_JMP_DISTANCE?
			    LONGJMP:LOCALJMP);
      break;
    case INSTR_TERM:
      bc->next = NULL;
      
      if (queue_isempty(todo))
	keepgoing = 0;
      else
	/* pluck off the next address from the todo queue */
	if (mseek(iscript, offset = queue_remove(todo), SEEK_SET)) {
	  sc_err_log("read_code: could not (condjmp) seek to %04x",
		  (unsigned)offset);
	  queue_free(todo);
	  return -1;
	}
      break;
    default:
      sc_err_log("read_code: received bad type from instr table: %d", isinstr_get_type(opcode));
      queue_free(todo);
      return -1;
    }
  }

  queue_free(todo);
  /* A OK */
  return 0;
}

/* free symtbl table hash on error. Will free
   everything that is non-NULL */
static void err_free_symtbl(HashTable *st) {
  HashEnum henum;
  IsSymTblEnt *st_entry;

  if (st == NULL)
    return;
  else
    henum = hashenum_create(st);

  while ((st_entry = hashenum_next(&henum)) != NULL) {
    if (st_entry->anims != NULL)
      objlist_freeall(st_entry->anims);
    if (st_entry->jumps != NULL)
      objlist_free(st_entry->jumps);
    free(st_entry);
  }

  free(st);
}

/* free byte code array on error */
static void err_free_code(IsInstr **carray, long csize) {
  int i;

  if (carray == NULL)
    return;

  for (i=0; i<csize; i++) {
    if (carray[i] != NULL) {
      if (carray[i]->args != NULL)
	free(carray[i]->args);
      free(carray[i]);
    }
  }

  free(carray);
}
	  

/* Initialize the bytecode array and symbol table. Assumes that the
   header hashtable has already been loaded. */
static int init_bytecode_and_symtbl(MFILE *iscript) {
  HashEnum enumeration = hashenum_create(headers);
  HashTable *new_symtbl;
  IsHeader *hdrptr;
  IsSymTblEnt *st_entry;
  IsInstr **next_ptr;
  IsInstr *last;
  int i, type;
  
  /* allocate the symtbl */
  symtbl = hashtable_new(SYMTBL_HASH_SIZE);

  /* allocate the code to be same number of 'byte codes' as the
     file. Eats up more memory than necessary, but it is easier
     this way :) Won't be used for long anyway */
  if (mseek(iscript, 0, SEEK_END)) {
    sc_err_log("init_bytecode_andsymtbl: could not seek to EOF");
    err_free_symtbl(symtbl);
    return -1;
  }
  codesize = mtell(iscript)+1; 
  code = calloc(codesize, sizeof(IsInstr *)); /* need to 0-out invalid entries */

  while((hdrptr = hashenum_next(&enumeration)) != NULL) {
    for (i=0; i<__header_types[hdrptr->type]; i++) {
      /* NULL pointer, means no animation for this entry */
      if (hdrptr->offsets[i] == NULL_UINT16) {
	/* make sure to indicate that this animno points to
	   nothing in the symtbl */
	hdrptr->st_entries[i] = NULL;
	continue;
      }
      if (mseek(iscript, hdrptr->offsets[i], SEEK_SET)) {
	sc_err_log("init_bytecode_andsymtbl: could not seek to %04x",
		(unsigned)mtell(iscript));
	err_free_symtbl(symtbl);
	err_free_code(code, codesize);
	return -1;
      }
      /* add a symtbl entry first */
      if (!(st_entry = hashtable_find(symtbl,hdrptr->offsets[i]))) {
	hashtable_insert(symtbl,hdrptr->offsets[i],
		   st_entry = new_symtbl_entry(hdrptr, i, NULL, HEADER));
	/* add a pointer to it */
	hdrptr->st_entries[i] = st_entry;
      } else {
	add_to_symtbl_entry(st_entry, hdrptr, i, NULL, HEADER);
	hdrptr->st_entries[i] = st_entry;
      }
      /* now start reading code */
      if (read_code(iscript, hdrptr, i) == -1) {
	err_free_symtbl(symtbl);
	err_free_code(code, codesize);
	return -1;
      }
    }
#ifndef __DEBUG
    /* don't need this anymore, we'll use symtbl pointers
       from now on (to abstract the offsets into movable labels) */
    free(hdrptr->offsets);
#endif
  }

  /* now go through the bytecode and fill in each instruction's
     pointer back to its symtbl entry (if it exists) and its
     'prev' instr pointer and the st_entry pointer to it
     and jump symbol if it has one. */
  last = NULL;
  next_ptr = NULL;
  for(i=0; i<codesize; i++)
    if (code[i] != NULL) {
      /* fill in the last next ptr if there is a placeholder
	 there */
      if (next_ptr != NULL && *next_ptr == (IsInstr *)-1)
	*next_ptr = code[i];
      code[i]->prev = last;
      /* find the symbol table entry that points to us and
	 mark a back pointer to it as well as a pointer from
	 it to us */
      if ((code[i]->st_entry = hashtable_find(symtbl, i)) != NULL)
	code[i]->st_entry->bc = code[i];
      type = isinstr_get_type(code[i]->opcode);
      /* if it is a cond jump or jump look up the symbol
	 Tbl entry it points to by its last argument (which
	 should be a offset) */
      if (type == INSTR_JMP || type == INSTR_COND_JMP)
	code[i]->jump = hashtable_find(symtbl, code[i]->args[code[i]->numargs-1]);
      if (code[i]->next != NULL) {
	last = code[i];
	next_ptr = &(code[i]->next);
      } else {
	last = NULL;
	next_ptr = NULL;
      }
#ifdef __DEBUG
      /* Sanity check 1: make sure all bytecodes either have a previous
	 code pointing to it or is pointed to by a symtbl entry (otherwise
	 we have a lone bytecode that isn't pointed to by anything
	 -- shouldn't be possible) */
      if (code[i]->prev == NULL && code[i]->st_entry == NULL)
	sc_err_fatal("init_bytecode_and_symtbl: failed sanity check, found byte code "
		  "at 0x%04x that isn't attached to anything", i);
      if (code[i]->prev != NULL && code[i]->prev->next != code[i])
	sc_err_fatal("init_bytecode_and_symtbl: failed sanity check, found byte code "
		  "at 0x%04x that has mismatched pointers with its previous", i);
      /* Santity Check for jumps to make sure their pointers are cosher */
      if (type == INSTR_JMP || type == INSTR_COND_JMP) {
	ObjListEnum lenum;
	int ok = 0;
	IsInstr *bc;
	if (code[i]->jump == NULL)
	  sc_err_fatal("init_bytecode_and_symtbl: found jump that points to NULL symtbl "
		    "entry at 0x%04x", i);
	lenum = objlistenum_create(code[i]->jump->jumps);
	while ((bc = objlistenum_next(&lenum)))
	  if (bc == code[i]) {
	    ok = 1;
	    break;
	  }
	if (!ok)
	  sc_err_fatal("init_bytecode_and_symtbl: found jump but its pointer to the symtbl "
		    "doesn't match the one that points back, at 0x%04x", i);
      }
#endif
    }

#ifdef __DEBUG
  /* saniy check number 2: make sure all the symtbl entries are pointed to
     by something so we don't get a memory leak */
  enumeration = hashenum_create(symtbl);
  while ((st_entry = hashenum_next(&enumeration)) != NULL) {
    int ok = 0;
    ObjListEnum list = objlistenum_create(st_entry->anims);
    IsAnim *anim_st;
    IsInstr *bc;
    while ((anim_st = objlistenum_next(&list)) != NULL)
      /* this is true if we have a back pointer to the header that
	 points to us */
      if (anim_st->header->st_entries[anim_st->animno] == st_entry)
	ok = 1;
    list = objlistenum_create(st_entry->jumps);
    while ((bc = objlistenum_next(&list)) != NULL) {
      /* found a jump that we point to that should point back
	 to us */
      if (bc->jump == st_entry)
	ok = 1;
      else
	sc_err_fatal("init_bytecode_and_symtbl: failed sanity check of symtbl entry %d, found a jump "
		  "but the instr's jump back ptr did not match ours", (int)st_entry->id);
    }
    if (!ok)
      sc_err_fatal("init_bytecode_and_symtbl: failed sanity check of symtbl, symtbl entry %d is not "
		"pointed to by any header or instruction", (int)st_entry->id);
  }
#endif

  /* Rehash the symtbl by ID instead of offset (this will be the final
     form that is used in the actual Iscript object, since offsets
     are now irrelevant) */
  new_symtbl  = hashtable_new(SYMTBL_HASH_SIZE);
  enumeration = hashenum_create(symtbl);
  while ((st_entry = hashenum_next(&enumeration)) != NULL)
    hashtable_insert(new_symtbl, st_entry->id, st_entry);
  hashtable_free(symtbl);
  symtbl = new_symtbl;

  /* don't need the big code array anymore, we have pointers to all our code
     links from the symtbl. */
#ifndef __DEBUG
  free(code); /* debugging routines still need this */  
#endif

  return 0;
}

/* Return a newly allocated symtbl entry:
   type is either HEADER, LOCALJMP, or LONGJMP */
static IsSymTblEnt *new_symtbl_entry(IsHeader *hdrptr, 
				      uint16 animno, 
				      IsInstr *jump_instr,
				      int type) 
{
  IsAnim *anim_entry;
  ObjList *list;
  IsSymTblEnt *st_entry = malloc(sizeof(IsSymTblEnt));

  anim_entry = malloc(sizeof(IsAnim));
  anim_entry->header = hdrptr;
  anim_entry->animno = animno;
  anim_entry->type   = type;

  /* make a list for the back pointers to headers */
  list = objlist_new();
  objlist_insert(list, anim_entry);
  st_entry->anims = list;

  /* make a list for back pointers to instructions which
     jump to here */
  list = objlist_new();
  if (jump_instr != NULL)
    objlist_insert(list, jump_instr);
  st_entry->jumps = list;

  st_entry->id = unique_id++;

  return st_entry;
}

static void add_to_symtbl_entry(IsSymTblEnt* st_entry, 
				IsHeader *hdrptr, 
				uint16 animno, 
				IsInstr *jump_instr, 
				int type) 
{
  IsAnim    *anim_entry;

  anim_entry = malloc(sizeof(IsAnim));
  anim_entry->header = hdrptr;
  anim_entry->animno = animno;
  anim_entry->type   = type;

  objlist_insert(st_entry->anims, anim_entry);
  if (jump_instr != NULL)
    objlist_insert(st_entry->jumps, jump_instr);
}

/* *** Debugging routines *** */

#ifdef __DEBUG
/* load the debug id->name hash */
void load__hash() {
  __hash = iscript_id_hash_new();
}

void free__hash() {
  iscript_id_hash_free(__hash);
  __hash = NULL;
}

/* super duper sanity check to make sure a full iscript object
   can be created reliably from the above functions */
int super_debug_function() {
  MFILE *iscript = mopen("data/iscript.bin");
  int i;
  HashEnum he;
  IsHeader *hi;
  IsSymTblEnt *ste;
  HashTable *mark;
  ObjQueue *q;

  if (iscript == NULL)
    sc_err_fatal("main: could not open iscript.bin");

  if (__hash == NULL) {
    mclose(iscript);
    return -1;
  }

  load_headers(iscript, BROODWAR);
  init_bytecode_and_symtbl(iscript);

  /*
  for(i=0; i<codesize; i++) 
    if (code[i] != NULL) { 
      print_bc(stderr, i, code[i]);
    }
  */

  /* Phat Test -- passed with flying colors: A+ :) */
  mark = hashtable_new(512);
  he = hashenum_create(headers);
  q = objqueue_new();
  while ((hi = hashenum_next(&he)) != NULL) {
    int i;
    for (i=0; i<__header_types[hi->type]; i++) {
      if ((ste = hi->st_entries[i]) != NULL) {
	IsInstr *bc = ste->bc;
	if (hashtable_find(mark, ste->id)) continue;
	hashtable_insert(mark, ste->id, (void *)1);
	while(bc->prev != NULL)
	  bc = bc->prev;
	while (bc != NULL) {
	  if (code[bc->offset] == NULL)
	    printf("<< code at %04x was read twice\n", (unsigned)bc->offset);
	  code[bc->offset] = NULL;
	  print_bc(stderr, bc->offset, bc);
	  if (bc->st_entry && !hashtable_find(mark,bc->st_entry->id))
	    hashtable_insert(mark, bc->st_entry->id, (void *)1);
	  if (isinstr_get_type(bc->opcode)==INSTR_COND_JMP) {
	    objqueue_insert(q, bc->jump);
	    bc = bc->next;
	  }
	  else if (isinstr_get_type(bc->opcode)==INSTR_JMP) {
	    objqueue_insert(q, bc->jump);
	    bc = NULL;
	  }
	  else if (isinstr_get_type(bc->opcode)==INSTR_NORMAL)
	    bc = bc->next;
	  else
	    bc = NULL;
	}
	while (!objqueue_isempty(q)) {
	  ste = objqueue_remove(q);
	  bc = ste->bc;
	  if (hashtable_find(mark, ste->id)) continue;
	  hashtable_insert(mark, ste->id, (void *)1);
	  while(bc->prev != NULL)
	    bc = bc->prev;
	  while (bc != NULL) {
	    if (code[bc->offset] == NULL)
	      printf("<< code at %04x was read twice\n", (unsigned)bc->offset);
	    code[bc->offset] = NULL;
	    print_bc(stderr, bc->offset, bc);
	    if (bc->st_entry && !hashtable_find(mark,bc->st_entry->id))
	      hashtable_insert(mark, bc->st_entry->id, (void *)1);
	    if (isinstr_get_type(bc->opcode)==INSTR_COND_JMP) {
	      objqueue_insert(q, bc->jump);
	      bc = bc->next;
	    }
	    else if (isinstr_get_type(bc->opcode)==INSTR_JMP) {
	      objqueue_insert(q, bc->jump);
	      bc = NULL;
	    }  
	    else if (isinstr_get_type(bc->opcode)==INSTR_NORMAL)
	      bc = bc->next;
	    else
	      bc = NULL;
	  }
	}
      }
    }
  }

  for (i=0; i<codesize; i++)
    if (code[i] != NULL)
      fprintf(stdout,":: code at %04x was never read\n", (unsigned)i);

  return 0;
}

/* Debugging routine. headers must already be initialized before
   you call this. Writes output to handle */
void print_headers(FILE *handle) {
  HashEnum enumeration = hashenum_create(headers);
  IsHeader *hdrptr;
  int i;

  while((hdrptr = hashenum_next(&enumeration)) != NULL) {
    fprintf(handle, "Header id: %04d offset: %04x type: %04x ", hdrptr->id, 
	    hdrptr->offset, hdrptr->type);
    fprintf(handle, "Offsets:");
    for (i=0; i<__header_types[hdrptr->type]; i++)
      fprintf(handle, " %04x", hdrptr->offsets[i]);
    fprintf(handle, "\n");
  }
}

/* debuging function for byte codes */
static void print_bc(FILE *handle, int start, IsInstr *bc) {
  int i;
  if (__hash == NULL) return;
  if (bc->st_entry) {
    HashTable *hash = __hash;
    ObjListEnum lenum, lenum2;
    IsAnim *anim_st;
    char *string;

    lenum = objlistenum_create(bc->st_entry->anims);
    while ((anim_st = objlistenum_next(&lenum)) != NULL) {
      lenum2 = objlistenum_create(hashtable_find(hash, anim_st->header->id)); 
      if ((string = objlistenum_next(&lenum2)) == NULL)
	fprintf(handle, ":unknown");
      else
	fprintf(handle, ":%s", string);
      while ((string = objlistenum_next(&lenum2)) != NULL)
	fprintf(handle, ":%s", string);
    }
    fprintf(handle,"[%s%4.4u]:\n", ((IsAnim *)bc->st_entry->anims->head)->type==HEADER?
	    "hdr":((IsAnim *)bc->st_entry->anims->head)->type==LONGJMP?"lng":"lcl",
	    (unsigned)bc->st_entry->id);
  }
  fprintf(handle, "%04x %9.9s: op %02x (%10.10s) ",
	  start,
	  isinstr_get_type(bc->opcode)==INSTR_NORMAL?
	  "normal":isinstr_get_type(bc->opcode)==INSTR_JMP?"jump":
	  isinstr_get_type(bc->opcode)==INSTR_COND_JMP?"condjmp":
	  "terminal",
	  bc->opcode, 
	  isinstr_get_name(bc->opcode));
  for (i=0; i< bc->numargs; i++) {
    if (i == bc->numargs-1 && 
	(isinstr_get_type(bc->opcode) == INSTR_JMP ||
	 isinstr_get_type(bc->opcode) == INSTR_COND_JMP))
      fprintf(handle, " [%s%4.4u]", 
	      ((IsAnim *)bc->jump->anims->head)->type==HEADER?
	      "hdr":((IsAnim *)bc->jump->anims->head)->type==LONGJMP?"lng":"lcl",
	      (unsigned)bc->jump->id);
    else
      fprintf(handle, " %04x", bc->args[i]);
  }
  fprintf(handle,"\n");
}
#endif
