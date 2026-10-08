#include "damgr/log.h"
#include "src/merge.c"
#include "src/validate.c"
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  damgr_log(INFO, "damgr started!");
  if (argc == 1 || argc > 2) {
    damgr_log(ERROR, "No damgr argument given, possible commands "
                     "are:\n\tdamgr validate\n\tdamgr merge");
    return EXIT_FAILURE;
  }

  int ret;
  int command_idx;
  if (strcmp(argv[1], "validate") == 0) {
    command_idx = 0;
  } else if (strcmp(argv[1], "merge") == 0) {
    command_idx = 1;
  } else {
    damgr_log(ERROR, "Not a valid damgr argument, possible commands "
                     "are:\n\tdamgr validate\n\tdamgr merge");
    return EXIT_FAILURE;
  }

  switch (command_idx) {
  case 0:
    damgr_log(INFO, "starting damgr validate!");
    ret = damgr_validate();
    break;
  case 1:
    damgr_log(INFO, "starting damgr merge!");
    ret = damgr_merge();
    break;
  }

  if (ret != EXIT_SUCCESS) {
    damgr_log(ERROR, "damgr %s failed...", argv[1]);
  } else {
    damgr_log(INFO, "damgr finished!");
  }
  return ret;
}
