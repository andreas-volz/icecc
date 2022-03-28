/* Just saved/unused functions right now */
#include <stdlib.h>
#include <string.h>
#include "scdef.h"
#include "icecc-utils.h"

#define ISCRIPT_ID_VAR    7 /* the number of the iscript ID variable in images.Dat */ 
#define IMAGES_STRING_VAR 0 /* the number of the GRP string variable in images.Dat */

/* Reads in all the images (GRP) names and hashes them keyed
   by iscript ID. The object is a ObjList of all the strings
   that match (more than one string mean more than one images.Dat
   entry uses the same iscript id) */
HashTable *iscript_id_hash_new() {
  Dat       *images_dat = dat_new(IMAGES_DAT_FILE_PATH, DAT_IMAGES);
  Tbl       *images_tbl = tbl_new(IMAGES_TBL_FILE_PATH);
  HashTable *iscript_id_hash = hashtable_new(256);
  int       i;

  /* read in all the images_tbl strings and put them in a hash
     keyed by their iscript IDs so we can look them up quickly later */
  for (i=dat_offsetof_varno(images_dat, ISCRIPT_ID_VAR); 
       i < dat_numberof_varno(images_dat, ISCRIPT_ID_VAR) +
	 dat_offsetof_varno(images_dat, ISCRIPT_ID_VAR); ++i) {
    uint32 iscid, stringid;
    char   *name, *tmp;
    ObjList *list;
    /* sanity check: make sure the entry has a string and that it is not
       already in the hashtable */
    iscid    = dat_get_value(images_dat, i, ISCRIPT_ID_VAR);
    stringid = dat_get_value(images_dat, i, IMAGES_STRING_VAR);
    if (dat_isvalid_entryno(images_dat, i, IMAGES_STRING_VAR) &&
	!hashtable_find(iscript_id_hash, iscid))
      {
	/* insert the string as newly allocated, indexing by the iscript ID in images.Dat */
	tmp = tbl_get_string(images_tbl, stringid);
	name = malloc(sizeof(char)*(strlen(tmp)+1));
	strcpy(name, tmp);
	list = objlist_new();
	objlist_insert(list,name);
	hashtable_insert(iscript_id_hash, iscid, list);
      }
    /* if we find another images entry with the same iscript id, but different name,
       tack it on to the end of the current name(s) so we have a record of them all */
    else if (dat_isvalid_entryno(images_dat, i, IMAGES_STRING_VAR)) {
      /* get the name and temporarily remove it from the hash */
      list = hashtable_find(iscript_id_hash, iscid);
      tmp  = tbl_get_string(images_tbl, stringid);
      name = malloc(sizeof(char)*(strlen(tmp)+1));
      strcpy(name, tmp);
      objlist_insert(list, name);
    }
  }

  dat_free(images_dat);
  tbl_free(images_tbl);

  return iscript_id_hash;
}

/* We need a little special work to free this hashtable since
   it's objects are lists and need to be freed recusively 
   node by node */
void iscript_id_hash_free(HashTable *iscript_id_hash) {
  HashEnum henum = hashenum_create(iscript_id_hash);
  ObjList *list;
  while((list = hashenum_next(&henum)) != NULL)
    objlist_freeall(list);
  hashtable_free(iscript_id_hash);
}
