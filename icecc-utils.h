/* icecc-utils.h */

#ifndef _ICECC_UTILS_H_
#define _ICECC_UTILS_H_

#include "scdef.h"

#define ISCRIPT_ID_VAR    7 /* the number of the iscript ID variable in images.Dat */ 
#define IMAGES_STRING_VAR 0 /* the number of the GRP string variable in images.Dat */

extern HashTable *iscript_id_hash_new();
extern void iscript_id_hash_free(HashTable *iscript_id_hash);

#endif
