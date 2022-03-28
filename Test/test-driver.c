/* iscript.test */

#include <stdlib.h>
#include <stdio.h>
#include "scdef.h"

int main(int argc, char **argv) {
  Iscript *iscript;

  /*load__hash();*/

  printf("STARCRAFT=%d BROODWAR=%d\n", STARCRAFT, BROODWAR);

  if (argc == 3) {
    printf("loading custom iscript.bin\n");
    iscript = iscript_new(argv[1], atoi(argv[2]));
  } else
    iscript = iscript_new("data/iscript.bin", BROODWAR);
  if (!iscript)
    sc_err_fatal("Couldn't open the bin file: %s", sc_get_err());
  /*super_debug_function();*/
  /*print_headers(stderr);
    load__hash();*/
  printf("saving...\n");
  if (iscript_save("test.bin",iscript) == -1)
    sc_err_fatal("Couldn't save test.bin: %s", sc_get_err());

  printf("freeing...\n");
  iscript_free(iscript);

  printf("done.\n");
  /*free__hash();*/
  return 0;
}
