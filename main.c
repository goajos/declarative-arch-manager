// #include "src/commands/init.c"
#include "src/commands/merge.c"
// #include "src/commands/update.c"
#include "src/logging.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  logger(LOG_INFO, "damgr started!");
  if (argc == 1 || argc > 2) {
    logger(LOG_ERROR, "No damgr argument given, possible commands "
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
    logger(LOG_ERROR, "Not a valid damgr argument, possible commands "
                      "are:\n\tdamgr init\n\tdamgr merge\n\tdamgr update");
    return EXIT_FAILURE;
  }

  int ret;
  switch (command_idx) {
  case 0:
    break;
  case 1:
    logger(LOG_INFO, "starting damgr merge...");
    if ((ret = damgr_merge()) != EXIT_SUCCESS) {
      logger(LOG_ERROR, "exit status: %s", ret);
      return ret;
    }
    break;
  case 2:
    break;
    // case 3:
    //     break;
  }

  logger(LOG_INFO, "damgr finished!");
  return EXIT_SUCCESS;
}
