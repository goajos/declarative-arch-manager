// #include "src/commands/init.c"
#include "src/commands/merge.c"
// #include "src/commands/update.c"
#include "damgr/logging.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  LOG(LOG_INFO, "damgr started!");
  if (argc == 1 || argc > 2) {
    LOG(LOG_ERROR, "No damgr argument given, possible commands "
                   "are:\n\tdamgr init\n\tdamgr merge\n\tdamgr update");
    return EXIT_FAILURE;
  }

  int command_idx;
  if (memcmp(argv[1], "init", 4) == 0) {
    command_idx = 0;
  } else if (memcmp(argv[1], "merge", 5) == 0) {
    command_idx = 1;
  } else if (memcmp(argv[1], "update", 6) == 0) {
    command_idx = 2;
    // else if (memcmp(argv[1], "validate", 8) == 0) command_idx = 3;
  } else {
    LOG(LOG_ERROR, "Not a valid damgr argument, possible commands "
                   "are:\n\tdamgr init\n\tdamgr merge\n\tdamgr update");
    return EXIT_FAILURE;
  }

  switch (command_idx) {
  case 0:
    break;
  case 1:
    LOG(LOG_INFO, "starting damgr merge...");
    if (damgr_merge() != EXIT_SUCCESS) {
      LOG(LOG_ERROR, "damgr %s failed...", argv[1]);
      return EXIT_FAILURE;
    }
    break;
  case 2:
    break;
    // case 3:
    //     break;
  }

  LOG(LOG_INFO, "damgr finished!");
  return EXIT_SUCCESS;
}
