#include <errno.h>
#include <stdio.h>
#include "scdef.h"
#include "dat.h"


int main() {
  Dat *mydat = dat_new("data/sprites.Dat", DAT_SPRITES);
  int i, code;
  const char *name;

  if (!mydat) 
    {
      printf("uk oh!\n");
      perror("hm...");
      exit(1);
    }

  for (i=0; i<get_dat_num_vars(mydat); i++) {
    int j;
    printf("%s: ", name = dat_nameof_varno(mydat, i));
    for (j=0; j<dat_numberof_varno(mydat, i)+dat_offsetof_varno(mydat, i); j++) {
      uint32 buf;
      if (!dat_isvalid_entryno(mydat,j,i)) continue;
      if (j%2==0)
	code =dat_get_value(&buf, mydat,j,i);
      else
	code =dat_get_value_by_varname(&buf, mydat,j,name);
      printf("(%d %d)", j, (int)buf);
    }
    printf("\n");
  }

  /*dat_set_value(mydat, 516, dat_indexof_varname(mydat, "SelectionCircleVerticalOffset"),
    42);*/
  dat_save("newdat.Dat", mydat);

  dat_free(mydat);
  dat_free(NULL);

  return 0;
}
