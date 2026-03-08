#include "damgr/log.h"
#include "damgr/utils.h"
#include <stdlib.h>

int damgr_init() {
  damgr_log(INFO, "running damgr init...");

  if (init_damgr_dir(true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (init_damgr_dir(false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
