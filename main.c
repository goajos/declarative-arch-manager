#include "damgr/log.h"
#include "src/commands/init.c"
#include "src/commands/merge.c"
#include "src/commands/update.c"
#include <stdlib.h>
#include <string.h>

// TODO: add proper documentation
int main(int argc, char *argv[]) {
  damgr_log(INFO, "damgr started!");
  if (argc == 1 || argc > 2) {
    damgr_log(ERROR, "No damgr argument given, possible commands "
                     "are:\n\tdamgr init\n\tdamgr merge\n\tdamgr update");
    return EXIT_FAILURE;
  }

  // TODO: reset hooks command? -> remove from state to reset
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
    if (damgr_init() != EXIT_SUCCESS) {
      damgr_log(ERROR, "damgr %s failed...", argv[1]);
      return EXIT_FAILURE;
    }
    break;
  case 1:
    damgr_log(INFO, "starting damgr merge...");
    if (damgr_merge() != EXIT_SUCCESS) {
      damgr_log(ERROR, "damgr %s failed...", argv[1]);
      return EXIT_FAILURE;
    }
    break;
  case 2:
    damgr_log(INFO, "starting damgr update...");
    if (damgr_update() != EXIT_SUCCESS) {
      damgr_log(ERROR, "damgr %s failed...", argv[1]);
      return EXIT_FAILURE;
    }
    break;
    // case 3:
    //     break;
  }

  damgr_log(INFO, "damgr finished!");
  return EXIT_SUCCESS;
}
