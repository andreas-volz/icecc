#include <stdio.h>
#include "scdef.h"

int main() {
  DatEntLst *d = datentlst_new("data/images.lst");
  int i;

  if (!d)
    sc_err_fatal("open failed: %s", sc_get_err());

  for (i=0; i<datentlst_numberof_strings(d); i++)
    printf("%s\n", datentlst_get_string(d, i));

  datentlst_free(d);

  return 0;
}
