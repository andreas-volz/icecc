/*
  IceCC. The main part of the actual decompiler.
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

/* $Id: icedc.c,v 1.28 2001/01/14 20:17:47 jp Exp $ */

/* #define __DEBUG2 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "scdef.h"
#include "icecc-share.h"

#define DEFAULT_OUTFILE "iscript.txt"

/* a mini structure to pack an associated name with the
   "winning" animation entry of the label (since there
   may be more than one animation entry associated with
   each label */
typedef struct SymTblName {
  char *name;   /* this needs to be freed */
  IsAnim *anim; /* this should not be freed -- it is part of the
		   Iscript main structure; read-only */
} SymTblName;

/* local functions */
static int init_resource_tables();
static void free_resource_tables();
static long isid_to_imagesent(long id);

static void init_symtbl_names(Iscript *iscript);
static void free_symtbl_names();
static char *symtblent_to_name(IsSymTblEnt *st_entry);
static int symtblent_to_type(IsSymTblEnt *st_entry);

static int animtype_gte_comparator(int t1, int t2);
static SymTblName *symtbl_get_new_stname(IsSymTblEnt *st_entry, 
					 ObjHashTable *used_names);

static void print_decompile_header(FILE *stream, char *infile);
static void get_args(int argc, char **argv, Queue *ids, 
		     char *infile, char *outfile, int *separate);
static void err_display_help();

/* general resource tables */

/* accessors for the tables */
#define isid_to_imagesent_list(id)  ((List *)hashtable_find(table_isid_to_imagesent_list, (id)))
#define imagesent_to_name(id)       (table_imagesent_to_name[(id)])
#define imagesent_to_isid(id)       (table_imagesent_to_isid[(id)])
#define imagesent_to_grpstring(id)  (table_imagesent_to_grpstring[(id)])
#define imagesent_to_isturning(id)  (table_imagesent_to_isturning[(id)])
#define spritesent_to_imagesent(id) (table_spritesent_to_imagesent[(id)])
#define spritesent_to_isid(id)      (imagesent_to_isid(spritesent_to_imagesent(id)))
#define spritesent_to_name(id)      (imagesent_to_name(spritesent_to_imagesent(id)))
#define spritesent_to_grpstring(id) (imagesent_to_grpstring(spritesent_to_imagesent(id)))
#define flingyent_to_spritesent(id) (table_flingyent_to_spritesent[(id)])
#define flingyent_to_imagesent(id)  (spritesent_to_imagesent(flingyent_to_spritesent(id)))
#define flingyent_to_isid(id)       (imagesent_to_isid(flingyent_to_imagesent(id)))
#define unitsent_to_flingyent(id)   (table_unitsent_to_flingyent[(id)])
#define unitsent_to_isid(id)        (flingyent_to_isid(unitsent_to_flingyent(id)))
#define sfxdataent_to_wavstring(id) (table_sfxdataent_to_wavstring[(id)])
#define weaponsent_to_name(id)      (table_weaponsent_to_name[(id)])
#define iscriptent_to_name(id)      (id>=tsize_iscriptent_to_name?NULL: \
                                     table_iscriptent_to_name[(id)])

/* simple accessors */
#define isid_to_name(id)      (isid_to_imagesent(id)==-1?"Unused": \
                               imagesent_to_name(isid_to_imagesent(id)))
#define isid_to_grpstring(id) (isid_to_imagesent(id)==-1?"unused": \
                               imagesent_to_grpstring(isid_to_imagesent(id)))

/* global resource tables used for lookups */
static HashTable  *table_isid_to_imagesent_list   = NULL;
static long       *table_imagesent_to_isid        = NULL;
static char       **table_imagesent_to_name       = NULL;
static size_t     tsize_imagesent_to_name         = 0;
static char       **table_imagesent_to_grpstring  = NULL;
static size_t     tsize_imagesent_to_grpstring    = 0;
static int        *table_imagesent_to_isturning   = NULL;
static long       *table_spritesent_to_imagesent  = NULL;
static long       *table_flingyent_to_spritesent  = NULL;
static long       *table_unitsent_to_flingyent    = NULL;
static char       **table_sfxdataent_to_wavstring = NULL;
static size_t     tsize_sfxdataent_to_wavstring   = 0;
static char       **table_weaponsent_to_name      = NULL;
static size_t     tsize_weaponsent_to_name        = 0;
static char       **table_iscriptent_to_name      = NULL;
static size_t     tsize_iscriptent_to_name        = 0;

/* Lookup iscript names from iscript.lst (0, default) or from a reference from images.dat/lst (1) */
static int iscript_id_lookup_type = 0;

/* hash that maps uniq_id to actual name */
static HashTable *table_symtbl_id_to_name         = NULL;
static int uniq_local_id = 0;
static int uniq_long_id  = 0;
static int uniq_jump_id  = 0;

/* ----- instr dispatching functions ----- */

#define iprint(file, instr) (instr_dispatch[(instr)->opcode].print((file), (instr)))
#define cprint(file, instr) (instr_dispatch[(instr)->opcode].comment((file), (instr)))

typedef void (print_instr)(FILE *fp, IsInstr *ip);
typedef void (print_comment)(FILE *fp, IsInstr *ip);

static print_instr iprint_normal;
static print_instr iprint_hex;
static print_instr iprint_playfram;
static print_instr iprint_jump;

static print_comment cprint_none;
static print_comment cprint_playfram;
static print_comment cprint_images;
static print_comment cprint_sprites;
static print_comment cprint_sfxdata_all;
static print_comment cprint_sfxdata_rand;
static print_comment cprint_weapons;

typedef struct InstrPrinter {
  print_instr   *print;
  print_comment *comment;
} InstrPrinter;

/* dispatch table */
static InstrPrinter instr_dispatch[] = {
  /* 00 */ {iprint_playfram, cprint_playfram},
  /* 01 */ {iprint_normal, cprint_none},
  /* 02 */ {iprint_normal, cprint_none},
  /* 03 */ {iprint_normal, cprint_none},
  /* 04 */ {iprint_normal, cprint_none},
  /* 05 */ {iprint_normal, cprint_none},
  /* 06 */ {iprint_normal, cprint_none},
  /* 07 */ {iprint_jump, cprint_none},
  /* 08 */ {iprint_normal, cprint_images},
  /* 09 */ {iprint_normal, cprint_images},
  /* 0a */ {iprint_normal, cprint_images},
  /* 0b */ {iprint_normal, cprint_none},
  /* 0c */ {iprint_normal, cprint_none},
  /* 0d */ {iprint_normal, cprint_images},
  /* 0e */ {iprint_normal, cprint_images},
  /* 0f */ {iprint_normal, cprint_sprites},
  /* 10 */ {iprint_normal, cprint_sprites},
  /* 11 */ {iprint_normal, cprint_sprites},
  /* 12 */ {iprint_normal, cprint_none},
  /* 13 */ {iprint_normal, cprint_sprites},
  /* 14 */ {iprint_normal, cprint_sprites},
  /* 15 */ {iprint_normal, cprint_sprites},
  /* 16 */ {iprint_normal, cprint_none},
  /* 17 */ {iprint_normal, cprint_none},
  /* 18 */ {iprint_normal, cprint_sfxdata_all},
  /* 19 */ {iprint_normal, cprint_sfxdata_rand},
  /* 1a */ {iprint_normal, cprint_sfxdata_all},
  /* 1b */ {iprint_normal, cprint_none},
  /* 1c */ {iprint_normal, cprint_sfxdata_rand},
  /* 1d */ {iprint_normal, cprint_none},
  /* 1e */ {iprint_jump, cprint_none},
  /* 1f */ {iprint_normal, cprint_none},
  /* 20 */ {iprint_normal, cprint_none},
  /* 21 */ {iprint_normal, cprint_none},
  /* 22 */ {iprint_normal, cprint_none},
  /* 23 */ {iprint_normal, cprint_none},
  /* 24 */ {iprint_normal, cprint_none},
  /* 25 */ {iprint_normal, cprint_none},
  /* 26 */ {iprint_normal, cprint_none},
  /* 27 */ {iprint_normal, cprint_none},
  /* 28 */ {iprint_normal, cprint_weapons},
  /* 29 */ {iprint_normal, cprint_none},
  /* 2a */ {iprint_normal, cprint_none},
  /* 2b */ {iprint_normal, cprint_none},
  /* 2c */ {iprint_normal, cprint_none},
  /* 2d */ {iprint_normal, cprint_none},
  /* 2e */ {iprint_normal, cprint_none},
  /* 2f */ {iprint_normal, cprint_none},
  /* 30 */ {iprint_normal, cprint_none},
  /* 31 */ {iprint_normal, cprint_none},
  /* 32 */ {iprint_normal, cprint_none},
  /* 33 */ {iprint_normal, cprint_none},
  /* 34 */ {iprint_normal, cprint_none},
  /* 35 */ {iprint_jump, cprint_none},
  /* 36 */ {iprint_normal, cprint_none},
  /* 37 */ {iprint_normal, cprint_none},
  /* 38 */ {iprint_normal, cprint_none},
  /* 39 */ {iprint_jump, cprint_none},
  /* 3a */ {iprint_jump, cprint_none},
  /* 3b */ {iprint_jump, cprint_none},
  /* 3c */ {iprint_jump, cprint_none},
  /* 3d */ {iprint_normal, cprint_none},
  /* 3e */ {iprint_normal, cprint_none},
  /* 3f */ {iprint_jump, cprint_none},
  /* 40 */ {iprint_normal, cprint_none},
  /* 41 */ {iprint_normal, cprint_none},
  /* 42 */ {iprint_normal, cprint_sprites},
  /* 43 */ {iprint_normal, cprint_none},
  /* 44 */ {iprint_normal, cprint_none},
};

/* instructions are printed with the print_instr
   function, then the print_comment function */

/* generic printer for an instruction */
static void iprint_normal(FILE *fp, IsInstr *ip) {
  int i;

  fprintf(fp, "\t%-15s\t", isinstr_name(ip));
  for (i=0; i<ip->numargs; i++)
    fprintf(fp, "%d%s", (int)ip->args[i],
	    i==ip->numargs-1?"":" ");
}

/* printer with arguments in hex */
static void iprint_hex(FILE *fp, IsInstr *ip) {
  int i;

  fprintf(fp, "\t%-15s\t", isinstr_name(ip));
  for (i=0; i<ip->numargs; i++)
    fprintf(fp, "0x%02x%s", (unsigned)ip->args[i],
	    i==ip->numargs-1?"":" ");
}

/* for frames, its easier to tell the frame set with
   hex notation */
static void iprint_playfram(FILE *fp, IsInstr *ip) {
  List *headers = isinstr_reached_by(ip);

  if (imagesent_to_isturning(isid_to_imagesent(headers->head)))
    iprint_hex(fp, ip);
  else
    iprint_normal(fp, ip);

  free(headers);
}

/* print a jump (or cond jump) */
static void iprint_jump(FILE *fp, IsInstr *ip) {
  int i;

  fprintf(fp, "\t%-15s\t", isinstr_name(ip));
  for (i=0; i<ip->numargs; i++) {
    /* last arg should be the label to jump to */
    if (i == ip->numargs-1)
      fprintf(fp, "%s", symtblent_to_name(isinstr_get_jump(ip)));
    else 
      fprintf(fp, "%d ", (int)ip->args[i]);
  }
}

/* no comment */
static void cprint_none(FILE *fp, IsInstr *ip) {
  fprintf(fp, "\n");
}

/* for playfram, display the frame-set (every 17)
   of the frame if a header this anim belongs to
   is a "turning" graphic */
static void cprint_playfram(FILE *fp, IsInstr *ip) {
  List *headers = isinstr_reached_by(ip);

  if (imagesent_to_isturning(isid_to_imagesent(headers->head)))
    fprintf(fp, "\t" COMMENT_START " frame set %d\n", ip->args[0]/17);
  else
    fprintf(fp, "\n");
}

/* displays the images.dat name and grp entry */
static void cprint_images(FILE *fp, IsInstr *ip) {
  fprintf(fp, "\t" COMMENT_START " %s (%s)\n", imagesent_to_name(ip->args[0]),
	  imagesent_to_grpstring(ip->args[0]));
}

/* ditto for sprites.dat */
static void cprint_sprites(FILE *fp, IsInstr *ip) {
  fprintf(fp, "\t" COMMENT_START " %s (%s)\n", spritesent_to_name(ip->args[0]),
	  spritesent_to_grpstring(ip->args[0]));
}

/* sfxdata, but assume all the arguments are sfxdata.dat
   pointers and print the references to all of them */
static void cprint_sfxdata_all(FILE *fp, IsInstr *ip) {
  int i;
  char *str;
  fprintf(fp, "\t" COMMENT_START);
  for (i=0; i<ip->numargs; i++) {
    str = sfxdataent_to_wavstring(ip->args[i]);
    fprintf(fp, " %s%s", str==NULL?"<NONE>":str,
	    i==ip->numargs-1?"":",");
  }
  fprintf(fp, "\n");
}

/* sfxdata, but assume all but the first are pointers,
   this is the case with the play random opcodes */
static void cprint_sfxdata_rand(FILE *fp, IsInstr *ip) {
  int i;
  char *str;
  fprintf(fp, "\t" COMMENT_START);
  for (i=1; i<ip->numargs; i++) {
    str = sfxdataent_to_wavstring(ip->args[i]);
    fprintf(fp, " %s%s", str==NULL?"<NONE>":str,
	    i==ip->numargs-1?"":",");
  }
  fprintf(fp, "\n");
}

/* weapons.dat pointer */
static void cprint_weapons(FILE *fp, IsInstr *ip) {
  fprintf(fp, "\t" COMMENT_START " %s\n", weaponsent_to_name(ip->args[0]));
}

/* ----- resource tables for name/id lookups ----- */

/* loads up all the resource lookup tables,
   returns 0 if everything loaded up fine, -1
   on error */
static int init_resource_tables() {
  Dat *dat;
  Tbl *tbl;
  DatEntLst *lst;
  DatEntLst *isclst;

  int i;

  /* First, allocate the images.dat based tables */
  dat = dat_new(path_images_dat, DAT_IMAGES);
  tbl = tbl_new(path_images_tbl);
  lst = datentlst_new(path_images_lst);
  isclst = datentlst_new(path_iscript_lst);

  if (dat==NULL || tbl==NULL || lst==NULL || isclst==NULL) {
    if (dat) dat_free(dat);
    if (tbl) tbl_free(tbl);
    if (lst) datentlst_free(lst);
    if (isclst) datentlst_free(isclst);
    free_resource_tables();
    return -1;
  }

  table_isid_to_imagesent_list = hashtable_new(256);
  table_imagesent_to_isid      = 
    malloc(dat_numberof_entries(dat)*sizeof(long));
  tsize_imagesent_to_name      =
    dat_numberof_entries(dat);
  table_imagesent_to_name      =
    malloc(dat_numberof_entries(dat)*sizeof(char *));
  tsize_imagesent_to_grpstring = 
    dat_numberof_entries(dat);
  table_imagesent_to_grpstring =
    malloc(dat_numberof_entries(dat)*sizeof(char *));
  table_imagesent_to_isturning =
    malloc(dat_numberof_entries(dat)*sizeof(int));
  
  for (i=0; i<dat_numberof_entries(dat); i++) {
    /* put it into the hashtable, or link it up
       with the list if there is already an IsId
       that matches */
    if (dat_isvalid_entryno(dat, i, 7)) {
      List *l;
      if ((l = hashtable_find(table_isid_to_imagesent_list, 
			      dat_get_value(dat, i, 7))) != NULL) 
	{
	  l = list_insert(l, i);
	  hashtable_remove(table_isid_to_imagesent_list, 
			   dat_get_value(dat, i, 7));
	} else {
	  l = list_new();
	  l = list_insert(l, i);
	}
      hashtable_insert(table_isid_to_imagesent_list,
		       dat_get_value(dat, i, 7), l);
    }
    /* load up the rest of the tables */
    table_imagesent_to_isid[i] =
      dat_isvalid_entryno(dat, i, 7)?
      dat_get_value(dat, i, 7) : -1;
    table_imagesent_to_name[i] =
      (char *)datentlst_get_string(lst, i);
    table_imagesent_to_grpstring[i] =
      dat_isvalid_entryno(dat, i, 0)?
      tbl_get_string(tbl, dat_get_value(dat, i, 0)) : NULL;
    table_imagesent_to_isturning[i] = 
      dat_isvalid_entryno(dat, i, 1)?
      dat_get_value(dat, i, 1) : -1;
  }

  /* go back an reallocate new space for the strings, since
     the old pointers to the ones in the tbl structures will
     get freed */
  for (i=0; i<dat_numberof_entries(dat); i++) {
    char *newstring;
    if (table_imagesent_to_name[i] != NULL) {
      newstring = 
	malloc(sizeof(char)*(strlen(table_imagesent_to_name[i])+1));
      strcpy(newstring, table_imagesent_to_name[i]);
      table_imagesent_to_name[i] = newstring;
    }
    if (table_imagesent_to_grpstring[i] != NULL) {
      newstring = 
	malloc(sizeof(char)*(strlen(table_imagesent_to_grpstring[i])+1));
      strcpy(newstring, table_imagesent_to_grpstring[i]);
      table_imagesent_to_grpstring[i] = newstring;
    }
  }

  dat_free(dat);
  tbl_free(tbl);
  datentlst_free(lst);

  /* now initialize the sprites.dat table */
  dat = dat_new(path_sprites_dat, DAT_SPRITES);
  if (dat==NULL) {
    free_resource_tables();
    return -1;
  }
  
  table_spritesent_to_imagesent = 
    malloc(sizeof(long)*dat_numberof_entries(dat));
  for (i=0; i<dat_numberof_entries(dat); i++)
    table_spritesent_to_imagesent[i] =
      dat_isvalid_entryno(dat, i, 0)?
      dat_get_value(dat, i, 0) : -1;

  dat_free(dat);

  /* now initialize the flingy.dat table */
  dat = dat_new(path_flingy_dat, DAT_FLINGY);
  if (dat==NULL) {
    free_resource_tables();
    return -1;
  }
  
  table_flingyent_to_spritesent = 
    malloc(sizeof(long)*dat_numberof_entries(dat));
  for (i=0; i<dat_numberof_entries(dat); i++)
    table_flingyent_to_spritesent[i] =
      dat_isvalid_entryno(dat, i, 0)?
      dat_get_value(dat, i, 0) : -1;

  dat_free(dat);

  /* now initialize the units.dat table */
  dat = dat_new(path_units_dat, DAT_UNITS);
  if (dat==NULL) {
    free_resource_tables();
    return -1;
  }
  
  table_unitsent_to_flingyent = 
    malloc(sizeof(long)*dat_numberof_entries(dat));
  for (i=0; i<dat_numberof_entries(dat); i++)
    table_unitsent_to_flingyent[i] =
      dat_isvalid_entryno(dat, i, 0)?
      dat_get_value(dat, i, 0) : -1;

  dat_free(dat);

  /* now do the sfxdata table */
  dat = dat_new(path_sfxdata_dat, DAT_SFXDATA);
  tbl = tbl_new(path_sfxdata_tbl);
  if (dat==NULL || tbl==NULL) {
    free_resource_tables();
    return -1;
  }

  tsize_sfxdataent_to_wavstring =
    dat_numberof_entries(dat);
  table_sfxdataent_to_wavstring =
    malloc(sizeof(char *)*dat_numberof_entries(dat));
  for (i=0; i<dat_numberof_entries(dat); i++)
    table_sfxdataent_to_wavstring[i] =
      dat_isvalid_entryno(dat, i, 0)?
      tbl_get_string(tbl, dat_get_value(dat, i, 0)) : NULL;

  for (i=0; i<dat_numberof_entries(dat); i++) {
    if (table_sfxdataent_to_wavstring[i] != NULL) {
      char *newstring = 
	malloc(sizeof(char)*(strlen(table_sfxdataent_to_wavstring[i])+1));
      strcpy(newstring, table_sfxdataent_to_wavstring[i]);
      table_sfxdataent_to_wavstring[i] = newstring;
    }
  }

  dat_free(dat);
  tbl_free(tbl);

  /* now do the weapons table */
  dat = dat_new(path_weapons_dat, DAT_WEAPONS);
  tbl = tbl_new(path_stat_txt_tbl);
  if (dat==NULL || tbl==NULL) {
    free_resource_tables();
    return -1;
  }
  
  tsize_weaponsent_to_name =
    dat_numberof_entries(dat);
  table_weaponsent_to_name =
    malloc(sizeof(char *)*dat_numberof_entries(dat));
  for (i=0; i<dat_numberof_entries(dat); i++)
    table_weaponsent_to_name[i] =
      dat_isvalid_entryno(dat, i, 0)?
      tbl_get_string(tbl, dat_get_value(dat, i, 0)) : NULL;

  for (i=0; i<dat_numberof_entries(dat); i++) {
    if (table_weaponsent_to_name[i] != NULL) {
      char *newstring = 
	malloc(sizeof(char)*(strlen(table_weaponsent_to_name[i])+1));
      strcpy(newstring, table_weaponsent_to_name[i]);
      table_weaponsent_to_name[i] = newstring;
    }
  }

  dat_free(dat);
  tbl_free(tbl);

  /* now initialize the iscript.lst table */
  tsize_iscriptent_to_name      =
    datentlst_numberof_strings(isclst);
  table_iscriptent_to_name      =
    malloc(tsize_iscriptent_to_name*sizeof(char *));

  for (i=0; i<tsize_iscriptent_to_name; i++) {
    char *newstring;
    if (datentlst_get_string(isclst, i) != NULL) {
      newstring = 
	malloc(sizeof(char)*(strlen(datentlst_get_string(isclst, i))+1));
      strcpy(newstring, datentlst_get_string(isclst, i));
      table_iscriptent_to_name[i] = newstring;
    }
  }

  datentlst_free(isclst);

  return 0;
}

/* free up all the resource lookup tables */
static void free_resource_tables() {
  int i;
  if (table_isid_to_imagesent_list != NULL) {
    HashEnum enumeration = hashenum_create(table_isid_to_imagesent_list);
    List *l;
    while ((l = hashenum_next(&enumeration)) != NULL)
      list_free(l);
    hashtable_free(table_isid_to_imagesent_list);
  }
  if (table_imagesent_to_isid != NULL)
    free(table_imagesent_to_isid);
  if (table_imagesent_to_name != NULL) {
    for (i=0; i<tsize_imagesent_to_name; i++)
      if (table_imagesent_to_name[i] != NULL)
	free(table_imagesent_to_name[i]);
    free(table_imagesent_to_name);
  }
  if (table_imagesent_to_grpstring != NULL) {
    for (i=0; i<tsize_imagesent_to_grpstring; i++)
      if (table_imagesent_to_grpstring[i] != NULL)
	free(table_imagesent_to_grpstring[i]);
    free(table_imagesent_to_grpstring);
  }
  if (table_imagesent_to_isturning != NULL)
    free(table_imagesent_to_isturning);
  if (table_spritesent_to_imagesent != NULL)
    free(table_spritesent_to_imagesent);
  if (table_flingyent_to_spritesent != NULL)
    free(table_flingyent_to_spritesent);
  if (table_unitsent_to_flingyent != NULL)
    free(table_unitsent_to_flingyent);
  if (table_sfxdataent_to_wavstring != NULL) {
    for (i=0; i<tsize_sfxdataent_to_wavstring; i++)
      if (table_sfxdataent_to_wavstring[i] != NULL)
	free(table_sfxdataent_to_wavstring[i]);
    free(table_sfxdataent_to_wavstring);
  }
  if (table_weaponsent_to_name != NULL) {
    for (i=0; i<tsize_weaponsent_to_name; i++)
      if (table_weaponsent_to_name[i] != NULL)
	free(table_weaponsent_to_name[i]);
    free(table_weaponsent_to_name);
  }
  if (table_iscriptent_to_name != NULL) {
    for (i=0; i<tsize_iscriptent_to_name; i++)
      if (table_iscriptent_to_name[i] != NULL)
	free(table_iscriptent_to_name[i]);
    free(table_iscriptent_to_name);
  }
  /* for our safety */
  table_isid_to_imagesent_list  = NULL;
  table_imagesent_to_isid       = NULL;
  table_imagesent_to_name       = NULL;
  tsize_imagesent_to_name       = 0;
  table_imagesent_to_grpstring  = NULL;
  tsize_imagesent_to_grpstring  = 0;
  table_imagesent_to_isturning  = NULL;
  table_spritesent_to_imagesent = NULL;
  table_flingyent_to_spritesent = NULL;
  table_unitsent_to_flingyent   = NULL;
  table_sfxdataent_to_wavstring = NULL;
  tsize_sfxdataent_to_wavstring = 0;
  table_weaponsent_to_name      = NULL;
  tsize_weaponsent_to_name      = 0;
  table_iscriptent_to_name      = NULL;
  tsize_iscriptent_to_name      = 0;
}

/* extract the "highest ranking" images entry from the resource tables
   that corresponds to this iscript ID, -1 if it doesn't exist */
static long isid_to_imagesent(long id) {
  List *l = hashtable_find(table_isid_to_imagesent_list, id);
  unsigned min = 0xFFFFFFFF;
  if (l == NULL)
    return -1;
  while (l != NULL) {
    if (l->head < min)
      min = l->head;
    l = l->tail;
  }
  return min;
}

/* ----- symtbl_to_name initialization ----- */

/* accessors */

/* returns the name we gave to this symtbl entry, NULL if not found */
static char *symtblent_to_name(IsSymTblEnt *st_entry) {
  SymTblName *st = hashtable_find(table_symtbl_id_to_name, st_entry->id);
  return st? st->name : NULL;
}

/* returns the type that we associated with this symtbl
   entry, LOCALJMP if not found (default) */
static int symtblent_to_type(IsSymTblEnt *st_entry) {
  SymTblName *st = hashtable_find(table_symtbl_id_to_name, st_entry->id);
  return (st && st->anim)? st->anim->type : LOCALJMP;
}

/* free the symtbl id to name hash */
static void free_symtbl_names() {
  HashEnum he = hashenum_create(table_symtbl_id_to_name);
  SymTblName *st;

  while ((st = hashenum_next(&he)) != NULL) {
    if (st != NULL) {
      free(st->name);
      free(st);
    }
  }

  if (table_symtbl_id_to_name != NULL)
    free(table_symtbl_id_to_name);
  table_symtbl_id_to_name = NULL;
}

/* initialize the global table_symtbl_id_to_name hash */
static void init_symtbl_names(Iscript *iscript) {
  /* keep track of names so we don't use them twice */
  ObjHashTable *used_names = objhashtable_new(512, string_hash, string_eq);
  HashEnum enumeration = hashenum_create(iscript->symtbl);
  IsSymTblEnt *st_entry;
  
  uniq_local_id = 0;
  uniq_long_id  = 0;
  uniq_jump_id  = 0;

  table_symtbl_id_to_name = hashtable_new(512);

  while ((st_entry = hashenum_next(&enumeration)) != NULL)
    hashtable_insert(table_symtbl_id_to_name, st_entry->id,
		     symtbl_get_new_stname(st_entry, used_names));

  objhashtable_free(used_names);
}

/* Returns true if t1 ">=" t2 */
static int animtype_gte_comparator(int t1, int t2) {
  /* long jumps always win */
  if (t1 == LONGJMP) return 1;
  if (t2 == LONGJMP) return 0;
  /* next is headers */
  if (t1 == HEADER) return 1;
  if (t2 == HEADER) return 0;
  /* last is local jumps, always GTE */
  return 1;
}

static SymTblName *symtbl_get_new_stname(IsSymTblEnt *st_entry, 
					 ObjHashTable *used_names)
{
  /* need to iterate through each anim ent */
  ObjListEnum enumeration = objlistenum_create(st_entry->anims);
  IsAnim   *anim, *winner = NULL;
  char buffer[512];
  SymTblName *result;

  /* find the "greatest" anim entry */
  while ((anim = objlistenum_next(&enumeration)) != NULL) {
    if (winner == NULL) {
      winner = anim;
      continue;
    }
    if (animtype_gte_comparator(anim->type, winner->type)) {
      winner = anim;
      continue;
    }
  }

  /* didn't find any anim back pointers... just call it 
     a jump */
  if (winner == NULL) {
    do {
      sprintf(buffer, "jump%02d", uniq_jump_id++);
    } while (objhashtable_find(used_names, buffer)); 
  } else {
    switch(winner->type) {
    case HEADER: 
      {
	char tmp[512];
	int id = 2;
	/* label name is "headerNameAnimName" */
	if (iscript_id_lookup_type == 0) {
		if (iscriptent_to_name(winner->header->id)) {
		  sprintf(buffer, "%s%s", iscriptent_to_name(winner->header->id),
			  animno_to_name(winner->animno));
		}
		else {
		  sprintf(buffer, "%s%d%s", "Unknown", winner->header->id,
			  animno_to_name(winner->animno));
		}
	}
	else {
	  sprintf(buffer, "%s%s", isid_to_name(winner->header->id),
		  animno_to_name(winner->animno));
	}
	/* check to see that the label is not already in use;
	   if it is, tack on a number to the end of it */
	strcpy(tmp, buffer);
	while (objhashtable_find(used_names, tmp)) {
	  sprintf(tmp, "%s%d", buffer, id++);
	}
	strcpy(buffer, tmp);
      }
      break;
    case LONGJMP:
      do {
	sprintf(buffer, "long%02d", uniq_long_id++);
      } while (objhashtable_find(used_names, buffer));
      break;
    default: /* localjmp */
      uniq_local_id = 0;
      do {
        if (iscript_id_lookup_type == 0) {
	      if (iscriptent_to_name(winner->header->id)) {
	        sprintf(buffer, "%sLocal%02d", iscriptent_to_name(winner->header->id),
	            uniq_local_id++);
		  }
		  else {
            sprintf(buffer, "%s%dLocal%02d", "Unknown", winner->header->id,
	            uniq_local_id++);
          }
		}
		else {
	      sprintf(buffer, "%sLocal%02d", isid_to_name(winner->header->id),
	          uniq_local_id++);
		}
      } while (objhashtable_find(used_names, buffer)); 
      break;
    }
  }

  /* copy it, mark it as used, and return it */
  result = malloc(sizeof(SymTblName));
  result->name = malloc(sizeof(char)*(strlen(buffer)+1));
  strcpy(result->name, buffer);
  result->anim = winner;
  objhashtable_insert(used_names, result->name, (void *)1);

  return result;
}

/* ----- other printing functions ----- */

/* print the iscript header hp with comments
   and all that good stuff. 0 if OK, -1 on error */
static int print_header(FILE *fp, Iscript *iscript, uint16 id) {
  IsHeader *hp = hashtable_find(iscript->headers, id);
  List *list;
  int i;

  if (hp == NULL) 
    return -1;

  list = isid_to_imagesent_list(hp->id);

  fprintf(fp, "\n" COMMENT_START
	  " -------------------------------------"
	  "---------------------------------------- " COMMENT_START "\n");

  /* comment on which images.dat entries use this iscript header */
  fprintf(fp, "# This header is used by images.dat entries:\n");
  while (list != NULL) {
    fprintf(fp, "# %03d %s (%s)\n", (int)list->head,
	    imagesent_to_name(list->head),
	    imagesent_to_grpstring(list->head));
    list = list->tail;
  }

  fprintf(fp, 
	  HEADER_TAG_START "\n"
	  ISCRIPT_ID_TAG "           \t%d\n"
	  ANIM_TYPE_TAG  "           \t%d\n",
	  (int)hp->id, (int)hp->type);

  for (i=0; i<iscript_numberof_anims(iscript, id); i++)
    fprintf(fp, "%-15s\t%s\n", animno_to_name(i),
	    hp->st_entries[i]? symtblent_to_name(hp->st_entries[i]) :
	    EMPTY_LABEL_STRING);

  fprintf(fp, HEADER_TAG_END "\n" COMMENT_START
	  " -------------------------------------"
	  "---------------------------------------- " COMMENT_START "\n\n");

  return 0;
}

/* print out the label associated with the symtbl entry.
   0 if OK, -1 on error */
static int print_label(FILE *fp, IsSymTblEnt *label) {
  const ObjList *list = symtblent_get_anim_list(label);

  if (list == NULL)
    return -1;

  /* put a extra newline above us id we're a long jump
     or header label -- no, instead do it after terminal instrs */
  /*if (symtblent_to_type(label) != LOCALJMP)
    fprintf(fp, "\n");*/

  /* only bother writing down comments if we can get here
     by more than one place -- disable for now, I don't
     really like it */
  /*if (objlist_length(list) > 1) {
    IsAnim *anim;
    ObjListEnum e = objlistenum_create((ObjList *)list);

    fprintf(fp, "# This label is arrived at by IsId's:\n");
    while ((anim = objlistenum_next(&e)) != NULL)
      fprintf(fp, "# %d (%s:%s)\n", anim->header->id, 
	      animno_to_name(anim->animno),
	      anim->type==HEADER?"header":
	      anim->type==LONGJMP?"longjmp":
	      "localjmp");
	      }*/

  /* print out the label name */
#ifdef __DEBUG2
  fprintf(fp, "[SYMTBLID%04d]", (int)label->id);
#endif
  fprintf(fp, "%s" LABEL_TERMINATOR "\n", symtblent_to_name(label));

  return 0;
}

/* print an iscript instruction */
static void print_instruction(FILE *fp, IsInstr *ip) {
  iprint(fp, ip);
  cprint(fp, ip);
  /* if terminal (end or jump), add an extra newline */
  if (ip->next == NULL)
    fprintf(fp, "\n");
}

/* print the iscript structure iscript to the file stream fp */
static int print_iscript(FILE *fp, Iscript *iscript) {
  IsIdEnum isid_enum = isidenum_create(iscript);
  ObjQueue *todo = objqueue_new();
  ObjQueue *longjmps = objqueue_new();
  HashTable *done = hashtable_new(512);
  uint16 id;
  int i;

  while ((id = isidenum_next(&isid_enum)) != (uint16)-1) {
    if (print_header(fp, iscript, id) == -1) {
      sc_err_log("print_iscript: failed to print header id %d", id);
      return -1;
    }

    for (i=0; i<iscript_numberof_anims(iscript, id); i++) {
      if (!iscript_isvalid_anim(iscript, id, i))
	continue;
      /* come back to long jumps later */
      if (symtblent_to_type(iscript_get_symtblent_by_animno(iscript, id, i)) == LONGJMP) {
	objqueue_insert(longjmps, iscript_get_symtblent_by_animno(iscript, id, i));
	continue;
      }

      objqueue_insert(todo, iscript_get_symtblent_by_animno(iscript, id, i));
      while (!objqueue_isempty(todo)) {
	IsInstr *bc;
	IsInstrEnum instr_enum = 
	  isinstrenum_create_from_symtblent(objqueue_remove(todo));
	isinstrenum_rewind(&instr_enum);
	while ((bc = isinstrenum_next(&instr_enum))) {
	  IsSymTblEnt *label = isinstr_get_label(bc), *jump;
	  if (label != NULL) {
	    if (hashtable_find(done, label->id))
	      break;
	    hashtable_insert(done, label->id, (void *)1);
	    if (print_label(fp, label) == -1) {
	      sc_err_log("print_iscript: failed to print label id %d", label->id);
	      return -1;
	    }
	  }
	  print_instruction(fp, bc);
	  if ((jump = isinstr_get_jump(bc)) != NULL) {
	    /* save long jumps to do later */
	    if (symtblent_to_type(jump) != LONGJMP)
	      objqueue_insert(todo, jump);
	    else
	      objqueue_insert(longjmps, jump);
	  }
	}
      }
    }    
  }

  if (!objqueue_isempty(longjmps))
    fprintf(fp, "\n" COMMENT_START
	    " --------------------------------------"
	    "---------------------------------------- " COMMENT_START "\n"
	    COMMENT_START " LONG JUMPS (these are usually shared "
	    "routines between many animations)         " COMMENT_START "\n"
	    COMMENT_START " --------------------------------------"
	    "---------------------------------------- " COMMENT_START "\n\n");
  
  /* now go through and print all the long jumps */
  while (!objqueue_isempty(longjmps)) {
    IsInstr *bc;
    IsInstrEnum instr_enum = 
      isinstrenum_create_from_symtblent(objqueue_remove(longjmps));
    isinstrenum_rewind(&instr_enum);
    while ((bc = isinstrenum_next(&instr_enum))) {
      IsSymTblEnt *label = isinstr_get_label(bc), *jump;
      if (label != NULL) {
	if (hashtable_find(done, label->id))
	  break;
	hashtable_insert(done, label->id, (void *)1);
	if (print_label(fp, label) == -1) {
	  sc_err_log("print_iscript: failed to print label id %d", label->id);
	  return -1;
	}
      }
      print_instruction(fp, bc);
      if ((jump = isinstr_get_jump(bc)) != NULL)
	objqueue_insert(todo, jump);
    }
  }

  /* done! :) */
  return 0;
}

int iscript_printer(FILE *fp, Iscript *ip) {
  if (init_resource_tables() == -1)
    return -1;
  init_symtbl_names(ip);
  if (print_iscript(fp, ip) == -1) {
    free_resource_tables();
    free_symtbl_names();
    return -1;
  }
  free_resource_tables();
  free_symtbl_names();
  return 0;
}

int main(int argc, char **argv) {
  Queue *ids = queue_new();
  char  infile[256], outfile[256];
  Iscript *iscript, *tmp;
  FILE *out;
  int separate = 0;

  memset(infile, '\0', 256);
  memset(outfile, '\0', 256);
  
  sc_register_prog_name(argv[0]);
  get_args(argc, argv, ids, infile, outfile, &separate);

  iscript = iscript_new(infile, BROODWAR);
  if (iscript == NULL)
    sc_err_fatal("could not open iscript: %s", strip_function_name(sc_get_err()));

  if (!queue_isempty(ids)) {
    Iscript *new;
    new = iscript_new_empty();

    while (!queue_isempty(ids)) {
      uint16 id = queue_remove(ids);

      tmp = iscript_extract_header(iscript, id);
      if (tmp == NULL)
	sc_err_fatal("%d is not a valid iscript ID in %s", id, infile);
      iscript_merge(new, tmp);
    }

    iscript_free(iscript);
    iscript = new;
  }
  queue_free(ids);

  if (separate) {
    tmp = iscript_separate_headers(iscript);
    iscript_free(iscript);
    iscript = tmp;
  }

  if (streq(outfile,"-"))
    out = stdout;
  else
    out = fopen(outfile, "w");
  if (out == NULL)
    sc_err_fatal("could not create output file: %s", strip_function_name(sc_get_err()));

  print_decompile_header(out, infile);
  if (iscript_printer(out, iscript) == -1) {
    remove(outfile);
    sc_err_fatal("decompiler failed: %s", strip_function_name(sc_get_err()));
  }
  iscript_free(iscript);
  fclose(out);

  return 0;
}

static void print_decompile_header(FILE *stream, char *infile) {
  time_t t = time(NULL);

  fprintf(stream, 
	  COMMENT_START " -------------------------------------------------"
	  "---------------------------- " COMMENT_START "\n"
	  COMMENT_START " This is a decompile of the iscript.bin file '%s'\n"
	  COMMENT_START " created on: %s"
	  COMMENT_START " -------------------------------------------------"
	  "---------------------------- " COMMENT_START "\n", infile,
	  ctime(&t));
}

static void err_display_help() {
  fprintf(stderr, 
	  "\nUsage: %s [-vh] [options] inputfile\n\n"
	  "Options:\n\n"
	  "  -d                 use the default iscript.bin as the input file\n"
	  "  -s                 separate headers (so no two share animations)\n"
	  "  -n                 get script names from images.lst, not iscript.lst\n"
	  "  -o ouputfile       decompile the inputfile to outputfile\n"
	  "  -c configfile      use the configuration file configfile\n"
	  "  -r configdir       use the configuration directory configdir\n"
	  "  -i iscriptidlist   only decompile the iscript IDs in iscriptidlist\n"
	  "  -m imageslist      only decompile iscript IDs associated with imageslist\n"
	  "  -p spriteslist     only decompile iscript IDs associated with spriteslist\n"
	  "  -f flingylist      only decompile iscript IDs associated with flingylist\n"
	  "  -u unitslist       only decompile iscript IDs associated with unitslist\n"
	  "\n",
	  sc_get_prog_name());
  exit(1);
}

static void get_args(int argc, char **argv, Queue *ids, 
		     char *infile, char *outfile, int *separate) {
  int i;
  char configdir[255];
  int usedefault = 0;
  int nomoreswitches = 0;
  Queue *images = queue_new();
  Queue *sprites = queue_new();
  Queue *flingy = queue_new();
  Queue *units  = queue_new();

  memset(configdir,'\0', 255);

  for (i=1; i<argc; i++) {
    if (streq(argv[i],"--")) {
      nomoreswitches = 1;
      continue;
    }
    if (argv[i][0] == '-' && !nomoreswitches) {
      while (argv[i] != NULL && strlen(argv[i]) > 1) {
	++argv[i];
	switch(argv[i][0]) {
	case 'h':
	  err_display_help();
	  break;
	case 'v':
	  display_version();
	  break;
	case 'd':
	  usedefault = 1;
	  break;
	case 's':
	  *separate = 1;
	  break;
	case 'n':
	  iscript_id_lookup_type = 1;
	  break;
	case 'o':
	  if (argv[i][1] != '\0')
	    strncpy(outfile, argv[i]+1, 255);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-o option expects a filename after it; e.g., -o output.txt");
	    strncpy(outfile, argv[i], 255);
	  }
	  argv[i] = NULL;
	  break;
	case 'c':
	  if (argv[i][1] != '\0')
	    set_config_file(argv[i]+1);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-c option expects a file after it; e.g., -c %s",
			   config_file_path);
	    set_config_file(argv[i]);
	  }
	  argv[i] = NULL;
	  break;
	case 'r':
	  if (argv[i][1] != '\0')
	    strncpy(configdir, argv[i]+1, 255);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-r option expects a directory after it; e.g., -r %s"
			   DIR_SEPARATOR "data", install_path);
	    strncpy(configdir, argv[i], 255);
	  }
	  argv[i] = NULL;
	  break;
	case 'i':
	  if (argv[i][1] != '\0')
	    commalist_to_queue(argv[i]+1, ids);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-i option expects a list of iscript IDs; e.g., -i 0,10,112");
	    commalist_to_queue(argv[i], ids);
	  }
	  argv[i] = NULL;
	  break;
	case 'm':
	  if (argv[i][1] != '\0')
	    commalist_to_queue(argv[i]+1, images);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-m option expects a list of images.dat entries; "
			   "e.g., -m 0,1,10");
	    commalist_to_queue(argv[i], images);
	  }
	  argv[i] = NULL;
	  break;
	case 'p':
	  if (argv[i][1] != '\0')
	    commalist_to_queue(argv[i]+1, sprites);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-p option expects a list of sprites.dat entries; "
			   "e.g., -p 0,1,10");
	    commalist_to_queue(argv[i], sprites);
	  }
	  argv[i] = NULL;
	break;
	case 'f':
	  if (argv[i][1] != '\0')
	    commalist_to_queue(argv[i]+1, flingy);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-f option expects a list of flingy.dat entries; "
			   "e.g., -s 0,1,10");
	    commalist_to_queue(argv[i], flingy);
	  }
	  argv[i] = NULL;
	  break;
	case 'u':
	  if (argv[i][1] != '\0')
	    commalist_to_queue(argv[i]+1, units);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-u option expects a list of units.dat entries; "
			   "e.g., -u 0,1,10");
	    commalist_to_queue(argv[i], units);
	  }
	  argv[i] = NULL;
	  break;
	default:
	  sc_err_warn("unknown option: %c", argv[i][0]);
	  break;
	}
      }
    } else {
      strncpy(infile, argv[i], 255);
      argv[i] = NULL;
    }
  }

  load_config_file();
  if (configdir[0] != '\0')
    set_config_dir(configdir);

  if (usedefault)
    strncpy(infile, path_iscript_bin, 255);
  if (infile[0] == '\0')
    err_display_help();
  if (outfile[0] == '\0')
    strncpy(outfile, DEFAULT_OUTFILE, 255);

  if (init_resource_tables() == -1)
    sc_err_fatal("loading config files failed: %s", strip_function_name(sc_get_err()));
  while (!queue_isempty(images))
    queue_insert(ids, imagesent_to_isid(queue_remove(images)));
  while (!queue_isempty(sprites))
    queue_insert(ids, spritesent_to_isid(queue_remove(sprites)));
  while (!queue_isempty(flingy))
    queue_insert(ids, flingyent_to_isid(queue_remove(flingy)));
  while (!queue_isempty(units))
    queue_insert(ids, unitsent_to_isid(queue_remove(units)));
  free_resource_tables();

  queue_free(images);
  queue_free(sprites);
  queue_free(flingy);
  queue_free(units);
}
