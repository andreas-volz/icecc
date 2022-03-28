#include <stdio.h>
#include <stdlib.h>
#include "tbl.h"

int main() {
  Tbl *mytbl = tbl_new("images.Tbl");
  int i;

  if (mytbl == NULL) {
    fprintf(stderr,"oh no!");
    exit(1);
  }

  for(i=0; i<tbl_get_size(mytbl); i++)
    printf("%s\n", tbl_get_string(mytbl, i));

  tbl_set_string(mytbl, tbl_get_size(mytbl), "whoopdidoo!");
  tbl_insert_string(mytbl, tbl_get_size(mytbl), "heehaw!");
  tbl_insert_string(mytbl, 1, "first post!");
  tbl_set_string(mytbl, 2, "second so naw!");
  tbl_insert_string(mytbl, 2, "hahaha");

  tbl_save("newtbl.Tbl",mytbl);
  tbl_free(mytbl);
  tbl_free(NULL);
  return 0;
}
