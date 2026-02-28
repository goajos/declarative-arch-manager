#include "damgr/log.h"
#include "damgr/utils.h"
#include <stdlib.h>

int damgr_init() {
  LOG(LOG_INFO, "running damgr init...");

  if (init_damgr_state_dir() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (init_damgr_config_dir() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
