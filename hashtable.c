/*
  IceCC. These are generic hashtable routines.
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

/* $Id: hashtable.c,v 1.10 2001/01/14 05:30:45 jp Exp $ */

#include <stdlib.h>
#include "scdef.h"
#include "hashtable.h"

static void hashbucket_free(HashBucket *bucket, int free_contents);
static HashBucket *hashenum_next_hashbucket(HashEnum *enumeration);
static void objhashbucket_free(ObjHashBucket *bucket, int free_contents);
static ObjHashBucket *objhashenum_next_hashbucket(ObjHashEnum *enumeration);

/* Create and return a new hashtable */
HashTable *hashtable_new(int size) {
  HashTable *newTable = malloc(sizeof(HashTable));
  newTable->size = size;
  newTable->data = calloc(size, sizeof(HashBucket *));
  return newTable;
}

/* Insert the data data into the table under the haskkey key */
void hashtable_insert(HashTable *table, long key, void *data){
  unsigned int location = key % table->size;
  HashBucket *newBucket = (HashBucket *) 
    malloc(sizeof(HashBucket));
  newBucket->next = table->data[location];
  newBucket->data = data;
  newBucket->key = key;
  table->data[location] = newBucket;
}

/* Find the data associated with hash key key and return it.
   Returns NULL if not found */
void *hashtable_find(HashTable *table, long key) {
  unsigned int location = key % table->size;
  HashBucket *lookAt = 
    table->data[location];
  while(lookAt != NULL){
    if(key == lookAt->key){
      return lookAt->data;
    }
    lookAt = lookAt->next;
  }
  return NULL;
}

/* Find the data indexed by key and remove it from the hashtable and
   then return it. You are responsible for freeing it. */
void *hashtable_remove(HashTable *table, long key) {
  unsigned int location = key % table->size;
  HashBucket **ptrLookAt = &(table->data[location]);
  HashBucket *lookAt     = table->data[location];
  while(lookAt != NULL){
    if(key == lookAt->key) {
      void *data = lookAt->data;
      *ptrLookAt = lookAt->next;
      free(lookAt);
      return data;
    }
    ptrLookAt = &(lookAt->next);
    lookAt = lookAt->next;
  }
  return NULL;
}

/* Free a hashbucket */
static void hashbucket_free(HashBucket *bucket, int free_contents) {
  /* first free my list */
  if (bucket->next != NULL)
    hashbucket_free(bucket->next, free_contents);
  if (free_contents)
    free(bucket->data);
  free(bucket);
}

/* Free all the objects inside this hashtable.
   Hashtable is as if it were just created */
void hashtable_freecontents(HashTable *table) {
  int i;

  for (i=0; i<table->size; i++) {
    if (table->data[i] != NULL) {
      hashbucket_free(table->data[i], 1);
      table->data[i] = NULL;
    }
  }
}

/* Free the table and all the objects in every one 
   of its buckets. You may call this after calling
   hashtable_freecontents (though this includes hashtable_freecontents) */
void hashtable_freeall(HashTable *table) {
  hashtable_freecontents(table);
  free(table->data);
  free(table);
}

/* Only free the table, do not free any of the objects
   inside it. hashtable_freecontents then hashtable_free is equivalent
   to hashtable_freeall */
void hashtable_free(HashTable *table) {
  int i;
  for (i=0; i<table->size; i++)
    if (table->data[i] != NULL)
      hashbucket_free(table->data[i], 0);
  free(table->data);
  free(table);
}

/* Creates an enumeration object out of a hastable. It is
   not a pointer. Do not free it. */
HashEnum hashenum_create(HashTable *table) {
  HashEnum enumeration;
  int i;

  enumeration.table = table;
  for (i=0; i < table->size; ++i) {
    if (table->data[i] == NULL)
      continue;
    enumeration.index       = i;
    enumeration.next_bucket = table->data[i];
    return enumeration;
  }

  enumeration.index = -1;
  enumeration.next_bucket = NULL;
  return enumeration;
}

/* helpter fn */
static HashBucket *hashenum_next_hashbucket(HashEnum *enumeration) {
  HashBucket *bucket;
  int i;

  if (enumeration->next_bucket == NULL)
    return NULL;
  
  bucket = enumeration->next_bucket;
  if (bucket->next != NULL) {
    enumeration->next_bucket = bucket->next;
    return bucket;
  }
  for (i=enumeration->index+1; i < enumeration->table->size; ++i) {
    if (enumeration->table->data[i] == NULL)
      continue;
    enumeration->index       = i;
    enumeration->next_bucket = enumeration->table->data[i];
    return bucket;
  }
  enumeration->index = -1;
  enumeration->next_bucket = NULL;
  return bucket;
}

/* Returns the next object in the enumeration
   (hash data pointer). Returns NULL if no more objects
   in the enumeration */
void *hashenum_next(HashEnum *enumeration) {
  HashBucket *bucket = hashenum_next_hashbucket(enumeration);
  return (bucket==NULL? NULL : bucket->data);
}

/* Returns the next key in the enumeration. 
   Returns (long)-1 if no more objects in the enumeration.
   Hopefully none of your keys have that value :) */
long hashenum_next_key(HashEnum *enumeration) {
  HashBucket *bucket = hashenum_next_hashbucket(enumeration);
  return (bucket==NULL? -1 : bucket->key);
}

/* Returns both the key and the data of the next bucket in
   the hash enumeration. Places values in key and data */
void hashenum_next_pair(HashEnum *enumeration, long *key, void **data) {
  HashBucket *bucket = hashenum_next_hashbucket(enumeration);
  if (bucket==NULL) {
    *key = 0xFFFF;
    *data = NULL;
  } else {
    *key = bucket->key;
    *data = bucket->data;
  }
}

/* *** ObjHashTable functions *** */

/* Create and return a new hashtable */
ObjHashTable *objhashtable_new(int size, int (*hash_fn)(void *),
			       int (*eq_fn)(void *, void *)) 
{
  ObjHashTable *newTable = malloc(sizeof(ObjHashTable));
  newTable->size = size;
  newTable->data = calloc(size, sizeof(ObjHashBucket *));
  newTable->hash_fn = hash_fn;
  newTable->eq_fn = eq_fn;
  return newTable;
}

/* Insert the data data into the table under the haskkey key */
void objhashtable_insert(ObjHashTable *table, void *key, void *data){
  unsigned int location = table->hash_fn(key) % table->size;
  ObjHashBucket *newBucket = malloc(sizeof(ObjHashBucket));
  newBucket->next = table->data[location];
  newBucket->data = data;
  newBucket->key = key;
  table->data[location] = newBucket;
}

/* Find the data associated with hash key key and return it.
   Returns NULL if not found */
void *objhashtable_find(ObjHashTable *table, void *key) {
  unsigned int location = table->hash_fn(key) % table->size;
  ObjHashBucket *lookAt = table->data[location];
  while(lookAt != NULL){
    if(table->eq_fn(key, lookAt->key)){
      return lookAt->data;
    }
    lookAt = lookAt->next;
  }
  return NULL;
}

/* Find the key associated with hash key key and return it.
   Returns NULL if not found */
void *objhashtable_findkey(ObjHashTable *table, void *key) {
  unsigned int location = table->hash_fn(key) % table->size;
  ObjHashBucket *lookAt = table->data[location];
  while(lookAt != NULL){
    if(table->eq_fn(key, lookAt->key)){
      return lookAt->key;
    }
    lookAt = lookAt->next;
  }
  return NULL;
}

/* Find the data indexed by key and remove it from the hashtable and
   then return it. You are responsible for freeing it. The key is
   freed by this fn if free_key is true */
void *objhashtable_remove(ObjHashTable *table, void *key, int free_key) {
  unsigned int location = table->hash_fn(key) % table->size;
  ObjHashBucket **ptrLookAt = &(table->data[location]);
  ObjHashBucket *lookAt     = table->data[location];
  while(lookAt != NULL){
    if(table->eq_fn(key, lookAt->key)) {
      void *data = lookAt->data;
      *ptrLookAt = lookAt->next;
      if (free_key) 
	free(lookAt->key);
      free(lookAt);
      return data;
    }
    ptrLookAt = &(lookAt->next);
    lookAt = lookAt->next;
  }
  return NULL;
}

/* Free a hashbucket */
static void objhashbucket_free(ObjHashBucket *bucket, int free_contents) {
  /* first free my list */
  if (bucket->next != NULL)
    objhashbucket_free(bucket->next, free_contents);
  if (free_contents) {
    free(bucket->key);
    free(bucket->data);
  }
  free(bucket);
}

/* Free all the objects inside this hashtable.
   Hashtable is as if it were just created */
void objhashtable_freecontents(ObjHashTable *table) {
  int i;

  for (i=0; i<table->size; i++) {
    if (table->data[i] != NULL) {
      objhashbucket_free(table->data[i], 1);
      table->data[i] = NULL;
    }
  }
}

/* Free the table and all the objects in every one 
   of its buckets. You may call this after calling
   hashtable_freecontents (though this includes hashtable_freecontents) */
void objhashtable_freeall(ObjHashTable *table) {
  objhashtable_freecontents(table);
  free(table->data);
  free(table);
}

/* Only free the table, do not free any of the objects
   inside it. hashtable_freecontents then hashtable_free is equivalent
   to hashtable_freeall */
void objhashtable_free(ObjHashTable *table) {
  int i;
  for (i=0; i<table->size; i++)
    if (table->data[i] != NULL)
      objhashbucket_free(table->data[i], 0);
  free(table->data);
  free(table);
}

/* Creates an enumeration object out of a hastable. It is
   not a pointer. Do not free it. */
ObjHashEnum objhashenum_create(ObjHashTable *table) {
  ObjHashEnum enumeration;
  int i;

  enumeration.table = table;
  for (i=0; i < table->size; ++i) {
    if (table->data[i] == NULL)
      continue;
    enumeration.index       = i;
    enumeration.next_bucket = table->data[i];
    return enumeration;
  }

  enumeration.index = -1;
  enumeration.next_bucket = NULL;
  return enumeration;
}

/* helpter fn */
static ObjHashBucket *objhashenum_next_hashbucket(ObjHashEnum *enumeration) {
  ObjHashBucket *bucket;
  int i;

  if (enumeration->next_bucket == NULL)
    return NULL;
  
  bucket = enumeration->next_bucket;
  if (bucket->next != NULL) {
    enumeration->next_bucket = bucket->next;
    return bucket;
  }
  for (i=enumeration->index+1; i < enumeration->table->size; ++i) {
    if (enumeration->table->data[i] == NULL)
      continue;
    enumeration->index       = i;
    enumeration->next_bucket = enumeration->table->data[i];
    return bucket;
  }
  enumeration->index = -1;
  enumeration->next_bucket = NULL;
  return bucket;
}

/* Returns the next object in the enumeration
   (hash data pointer). Returns NULL if no more objects
   in the enumeration */
void *objhashenum_next(ObjHashEnum *enumeration) {
  ObjHashBucket *bucket = objhashenum_next_hashbucket(enumeration);
  return (bucket==NULL? NULL : bucket->data);
}

/* Returns the next key in the enumeration. */
void *objhashenum_next_key(ObjHashEnum *enumeration) {
  ObjHashBucket *bucket = objhashenum_next_hashbucket(enumeration);
  return (bucket==NULL? NULL : bucket->key);
}

/* Returns both the key and the data of the next bucket in
   the hash enumeration. Places values in key and data */
void objhashenum_next_pair(ObjHashEnum *enumeration, void **key, void **data) {
  ObjHashBucket *bucket = objhashenum_next_hashbucket(enumeration);
  if (bucket==NULL) {
    *key = NULL;
    *data = NULL;
  } else {
    *key = bucket->key;
    *data = bucket->data;
  }
}
