#include "../logging.h"
#include "../state.h"
#include <stdlib.h>

int damgr_merge() {
  logger(LOG_INFO, "running damgr merge...");

  struct config old_config = {};
  if (get_config(&old_config, true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  };
  struct config new_config = {};
  if (get_config(&new_config, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  };

  return EXIT_SUCCESS;
}
