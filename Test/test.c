#include <stdio.h>
#include <stdlib.h>
#include "ice-utils.h"
#include "queue.h"
#include "hashtable.h"

static unsigned header_hash_fn(uint16 id) {
  return id;
}

static int header_eq_fn(uint16 a, uint16 b) {
  return a == b;
}

int main() {
  Queue *q = queue_new();
  HashTable *h = hashtable_new(512, header_hash_fn, header_eq_fn);
  HashEnum e;
  IsHeader *hp; 
  int i, j;

  printf("Testing Queue...\n");
  queue_insert(q, 1);
  queue_insert(q, 2);
  queue_insert(q, 3);
  queue_insert(q, 4);
  queue_insert(q, 5);
  queue_remove(q);
  queue_insert(q, 1);
  queue_insert(q, 2);
  queue_insert(q, 3);
  queue_insert(q, 4);
  queue_remove(q);
  queue_remove(q);
  queue_insert(q, 5);
  while (!queue_isempty(q))
    printf("%u ", (unsigned)queue_remove(q));
  printf("\n");

  printf("Testing Hash...\n");

  for (i=1; i<1024; i++)
    hashtable_insert(h, i, malloc(sizeof(IsHeader)));

  j = 0;
  e = hashenum_create(h);
  while((hp = hashenum_next(&e)) != NULL) {
    hp->offset = header_hash_fn(j);
    for (i=0; i<32; i++)
      hp->offsets[i] = i;
    hp->name = malloc(sizeof(char)*64);
    sprintf(hp->name, "I am number %d", j++);
  }

  for (j=10; j<800; j+=31) {
    hp = hashtable_find(h, (uint16)j);
    if (hp == NULL) {
      printf("Did not find key %d\n", j);
      continue;
    }
    printf("Found key %d - offset: %d name: %s\n", j, hp->offset, hp->name);
    printf("Offsets:");
    for (i=0; i<32; i++)
      printf(" %d", hp->offsets[i]);
    printf("\n");
  }

  printf("*** Super Test ***\n");

  e = hashenum_create(h);
  while((hp = hashenum_next(&e)) != NULL) {
    printf("%d ", hp->offset%1024 );
  }
  printf("\n");

  hashtable_freecontents(h);
  free(h);
  free(q);

  return 0;
}
