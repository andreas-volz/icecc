/* hashtable.h */

#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include "scdef.h"

#ifndef NULL
#define NULL ((void *) 0)
#endif

typedef struct HashBucket {
  long   key;
  void   *data;
  struct HashBucket *next;
} HashBucket;

/* This is not the most universal hashtable in the world;
   to be efficient, you're keys must be evenly distributed
   within the size bounds (which fits my purposes, so I
   don't need to clutter thing up with a callback function) */
typedef struct HashTable {
  struct HashBucket **data;
  int size;
} HashTable;

typedef struct HashEnum {
  HashTable  *table;
  int        index;
  HashBucket *next_bucket;
} HashEnum;

/* more general hashtable */

typedef struct ObjHashBucket {
  void   *key;
  void   *data;
  struct ObjHashBucket *next;
} ObjHashBucket;

typedef struct ObjHashTable {
  struct ObjHashBucket **data;
  int size;
  int (*hash_fn)(void *);
  int (*eq_fn)(void *, void *);
} ObjHashTable;

typedef struct ObjHashEnum {
  ObjHashTable  *table;
  int        index;
  ObjHashBucket *next_bucket;
} ObjHashEnum;

extern HashTable *hashtable_new(int size);
extern void hashtable_insert(HashTable *table, long key, void *data);
extern void *hashtable_find(HashTable *table, long key);
extern void *hashtable_remove(HashTable *table, long key);
extern void hashtable_freecontents(HashTable *table);
extern void hashtable_freeall(HashTable *table);
extern void hashtable_free(HashTable *table);

extern HashEnum hashenum_create(HashTable *table);
extern void *hashenum_next(HashEnum *enumeration);
extern long hashenum_next_key(HashEnum *enumeration);
extern void hashenum_next_pair(HashEnum *enumeration, long *key, void **data);

extern ObjHashTable *objhashtable_new(int size, int (*hash_fn)(void *), 
				      int (*eq_fn)(void *, void *));
extern void objhashtable_insert(ObjHashTable *table, void *key, void *data);
extern void *objhashtable_find(ObjHashTable *table, void *key);
extern void *objhashtable_findkey(ObjHashTable *table, void *key);
extern void *objhashtable_remove(ObjHashTable *table, void *key, int free_key);
extern void objhashtable_freecontents(ObjHashTable *table);
extern void objhashtable_freeall(ObjHashTable *table);
extern void objhashtable_free(ObjHashTable *table);

extern ObjHashEnum objhashenum_create(ObjHashTable *table);
extern void *objhashenum_next(ObjHashEnum *enumeration);
extern void *objhashenum_next_key(ObjHashEnum *enumeration);
extern void objhashenum_next_pair(ObjHashEnum *enumeration, void **key, void **data);

#endif

