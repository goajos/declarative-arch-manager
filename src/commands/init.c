#include "damgr/log.h"
#include "damgr/utils.h"
#include <stdlib.h>

int damgr_init() {
  damgr_log(INFO, "running damgr init...");

  char *user = get_user();
  if (user == nullptr) {
    return EXIT_FAILURE;
  }
  if (init_damgr_dir(user, true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (init_damgr_dir(user, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
