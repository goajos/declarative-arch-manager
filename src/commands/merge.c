#include "damgr/logging.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <stdlib.h>

int damgr_merge() {
  LOG(LOG_INFO, "running damgr merge...");

  char fidbuf[PATH_MAX];
  struct config old_config = {};
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr", get_user());
  int ret = is_damgr_state_dir_empty(fidbuf);
  if (ret == EXIT_FAILURE) {
    LOG(LOG_ERROR, "failed to open state directory: %s", fidbuf);
    return EXIT_FAILURE;
  }
  if (ret == 2) { // only "." and ".." dirs found
    LOG(LOG_INFO, "no state to parse, the state directory is empty: %s",
        fidbuf);
  } else {
    if (get_config(&old_config, true) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    };
  }
  struct config new_config = {};
  if (get_config(&new_config, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  };

  if (old_config.active_host.name != nullptr) {
    if (get_host(&old_config.active_host, true) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }
  if (get_host(&new_config.active_host, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
