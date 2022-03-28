/*
  IceCC. This is a tiny helper program for identifying stuff.
  Copyright (C) 2000 Jeffrey Pang <jp@magnus99.dhs.org>

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

/* $Id: datutil.c,v 1.5 2001/01/02 02:13:50 jp Exp $ */


/* Mini utility program to get the name of an entry in
   a certain Dat file. Run it as:

   datutil <type> <entryno>

   where type is either images, sfxdata, sprites, units, etc.,
   and entryno is the entry number you want and it will give
   you the cononical name from the appropriate Tbl file. Type may
   also be the special tag 'iscript' in which case it looks up the
   images.Dat Tbl entry(s) associated with the iscript ID */

#include <stdio.h>
#include <stdlib.h>
#include "scdef.h"
#include "icecc-utils.h"

int main(int argc, char **argv) {
  Dat   *mydat;
  Tbl   *mytbl;
  char  *name;
  int   entryno;

  if (argc != 3)
    sc_err_fatal("usage: datutil <type> <entryno>");

  entryno = strtol(argv[2], NULL, 0);

  if (streq("images",argv[1])) {
  images:
    if ((mydat = dat_new(IMAGES_DAT_FILE_PATH,DAT_IMAGES)) == NULL)
      sc_err_fatal("can't open/create Dat for %s", argv[1]);
    if ((mytbl = tbl_new(IMAGES_TBL_FILE_PATH)) == NULL)
      sc_err_fatal("can't open/create Tbl for %s", argv[1]);
    if (dat_isvalid_entryno(mydat, entryno, 0))
      name = tbl_get_string(mytbl, dat_get_value(mydat, entryno, 0));
    else
      sc_err_fatal("could not find entry %d in Dat", entryno);
    printf("%s", name?name:"Tbl string not found");
  } else if (streq("sfxdata",argv[1])) {
    if ((mydat = dat_new(SFXDATA_DAT_FILE_PATH,DAT_SFXDATA)) == NULL)
      sc_err_fatal("can't open/create Dat for %s", argv[1]);
    if ((mytbl = tbl_new(SFXDATA_TBL_FILE_PATH)) == NULL)
      sc_err_fatal("can't open/create Tbl for %s", argv[1]);
    if (dat_isvalid_entryno(mydat, entryno, 0))
      name = tbl_get_string(mytbl, dat_get_value(mydat, entryno, 0));
    else
      sc_err_fatal("could not find entry %d in Dat", entryno);
    printf("%s", name?name:"Tbl string not found");
  } else if (streq("units",argv[1])) {
    /* units doesn't have a pointer to the unit name, so we'll just
       go to its graphic -- the unit name pointer is in the executable */
    if ((mydat = dat_new(UNITS_DAT_FILE_PATH,DAT_UNITS)) == NULL)
      sc_err_fatal("can't open/create Dat for %s", argv[1]);
    if ((mytbl = tbl_new(STAT_TXT_TBL_FILE_PATH)) == NULL)
      sc_err_fatal("can't open/create Tbl for %s", argv[1]);
    if (dat_isvalid_entryno(mydat, entryno, 0))
      entryno = dat_get_value(mydat, entryno, 0);
    else
      sc_err_fatal("could not find entry %d in Dat", entryno);
    goto flingy;
  } else if (streq("weapons",argv[1])) {
    if ((mydat = dat_new(WEAPONS_DAT_FILE_PATH,DAT_WEAPONS)) == NULL)
      sc_err_fatal("can't open/create Dat for %s", argv[1]);
    if ((mytbl = tbl_new(STAT_TXT_TBL_FILE_PATH)) == NULL)
      sc_err_fatal("can't open/create Tbl for %s", argv[1]);
    if (dat_isvalid_entryno(mydat, entryno, 0))
      name = tbl_get_string(mytbl, dat_get_value(mydat, entryno, 0));
    else
      sc_err_fatal("could not find entry %d in Dat", entryno);
    printf("%s", name?name:"Tbl string not found");
  } else if (streq("sprites",argv[1])) {
  sprites:
    if ((mydat = dat_new(SPRITES_DAT_FILE_PATH,DAT_SPRITES)) == NULL)
      sc_err_fatal("can't open/create Dat for %s", argv[1]);
    if (dat_isvalid_entryno(mydat, entryno, 0))
      entryno = dat_get_value(mydat, entryno, 0);
    else
      sc_err_fatal("could not find sprites entry %d in Dat", entryno);
    dat_free(mydat);
    goto images; /* so sue me :P */
  } else if (streq("flingy",argv[1])) {
  flingy:
    if ((mydat = dat_new(FLINGY_DAT_FILE_PATH,DAT_FLINGY)) == NULL)
      sc_err_fatal("can't open/create Dat for %s", argv[1]);
    if (dat_isvalid_entryno(mydat, entryno, 0))
      entryno = dat_get_value(mydat, entryno, 0);
    else
      sc_err_fatal("could not find flingy entry %d in Dat", entryno);
    dat_free(mydat);
    goto sprites;
  } else if (streq("iscript", argv[1])) {
    HashTable *idhash = iscript_id_hash_new();
    ObjListEnum le = objlistenum_create(hashtable_find(idhash, entryno));
    char *name;
    while ((name = objlistenum_next(&le)) != NULL)
      printf("%s ", name);
    iscript_id_hash_free(idhash);
  } else
    sc_err_fatal("unrecognized type");

  printf("\n");
  if (!streq("iscript", argv[1]))
    dat_free(mydat);
  return 0;
}
  
