#include "damgr/log.h"
#include "src/commands/merge.c"
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  damgr_log(INFO, "damgr started!");
  if (argc == 1 || argc > 2) {
    damgr_log(ERROR, "No damgr argument given, possible commands "
                     "are:\n\tdamgr init\n\tdamgr merge\n\tdamgr update");
    return EXIT_FAILURE;
  }

  // TODO: reimplement init and update!
  // TODO: reset hooks command? -> remove from state to reset
  int ret;
  int command_idx;
  if (memcmp(argv[1], "init", 4) == 0) {
    command_idx = 0;
  } else if (memcmp(argv[1], "merge", 5) == 0) {
    command_idx = 1;
  } else if (memcmp(argv[1], "update", 6) == 0) {
    command_idx = 2;
    // TODO: add validate command?
    // else if (memcmp(argv[1], "validate", 8) == 0) command_idx = 3;
  } else {
    damgr_log(ERROR, "Not a valid damgr argument, possible commands "
                     "are:\n\tdamgr init\n\tdamgr merge\n\tdamgr update");
    return EXIT_FAILURE;
  }

  switch (command_idx) {
  case 0:
    damgr_log(INFO, "starting damgr init..");
    ret = EXIT_SUCCESS;
    break;
  case 1:
    damgr_log(INFO, "starting damgr merge...");
    ret = damgr_merge();
    break;
  case 2:
    damgr_log(INFO, "starting damgr update...");
    ret = EXIT_SUCCESS;
    break;
  }

  if (ret != EXIT_SUCCESS) {
    damgr_log(ERROR, "damgr %s failed...", argv[1]);
  } else {
    damgr_log(INFO, "damgr finished!");
  }
  return ret;
}
