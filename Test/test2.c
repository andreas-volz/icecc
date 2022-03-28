#include <stdio.h>
#include "scdef.h"

int main() {
  HashTable *c = hashtable_new(20);
  HashEnum b;
  ObjList *l = objlist_new();
  char *g;
  ObjListEnum o;

  hashtable_insert(c, 1, "1pp");
  hashtable_insert(c, 2, "2pp");
  hashtable_insert(c, 3, "3pp");
  hashtable_insert(c, 4, "4pp");
  hashtable_insert(c, 6, "6pp");
  hashtable_insert(c, 8, "8pp");
  hashtable_insert(c, 9, "9pp");
  
  b = hashenum_create(c);
  while ((g = hashenum_next(&b)) != NULL)
    printf("%s\n", g);

  hashtable_free(c);

  objlist_insert(l, "a");
  objlist_insert(l, "b");
  objlist_insert(l, "c");
  objlist_insert(l, "d");
  objlist_insert(l, "e");

  o = objlistenum_create(l);

  while((g = objlistenum_next(&o)) != NULL)
    printf("%s\n", g);

  objlist_free(l);

  return 0;
}
  
