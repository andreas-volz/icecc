#include <stdio.h>
#include <string.h>
#include <math.h>
#include "scdef.h"

int eq(void *a, void *b) {
  return !strcmp(a,b);
}

int hash(void *a) {
  int len = strlen(a);
  int i, result = 0;

  for (i=0; i<len; i++)
    result += ((char *)a)[i]*pow(37,i);

  return result;
}

int main() {
  ObjHashTable *h = objhashtable_new(100, hash, eq);
  ObjHashEnum e;
  char *s;

  objhashtable_insert(h, "hi", "bye");
  objhashtable_insert(h, "black", "white");
  objhashtable_insert(h, "good", "bad");

  e = objhashenum_create(h);

  while ((s = objhashenum_next(&e)) != NULL)
    printf("%s\n", s);

  printf("%s\n", objhashtable_find(h, "hi"));
  printf("%s\n", objhashtable_find(h, "black"));
  printf("%s\n", objhashtable_find(h, "good"));
  printf("%s\n", objhashtable_find(h, "none")?"found":"not found");

  objhashtable_free(h);
  return 0;
}
