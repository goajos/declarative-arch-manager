#include "damgr/actions.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <stdlib.h>
// #include <string.h>

int damgr_merge() {
  damgr_log(INFO, "running damgr merge...");

  char *user = damgr_get_user();
  if (user == nullptr) {
    return EXIT_FAILURE;
  }
  int ret;
  char fidbuf[damgr_path_max];
  Damgr_Config old_config = {};
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr", user);
  ret = damgr_is_state_dir_empty(fidbuf);
  if (damgr_is_state_dir_empty(fidbuf) == EXIT_FAILURE) {
    damgr_log(ERROR, "failed to open state directory: %s", fidbuf);
  }
  if (ret == 2) { // only "." and ".." dirs found
    damgr_log(INFO, "no state to parse, the state directory is empty: %s",
              fidbuf);
  } else {
    if (damgr_read_config(user, &old_config, true) != EXIT_SUCCESS) {
      goto cleanup;
    };
  }
  ret = EXIT_FAILURE;
  Damgr_Config config = {};
  if (damgr_read_config(user, &config, false) != EXIT_SUCCESS) {
    goto cleanup;
  };

  if (old_config.active_host.name != nullptr) {
    if (damgr_read_host(user, &old_config, true) != EXIT_SUCCESS) {
      goto cleanup;
    }
    for (size_t i = 0; i < old_config.active_host.modules.count; ++i) {
      if (damgr_read_module(user, &old_config, i, true) != EXIT_SUCCESS) {
        goto cleanup;
      }
    }
  }
  if (damgr_read_host(user, &config, false) != EXIT_SUCCESS) {
    goto cleanup;
  }
  for (size_t i = 0; i < config.active_host.modules.count; ++i) {
    if (damgr_read_module(user, &config, i, false) != EXIT_SUCCESS) {
      goto cleanup;
    }
  }

  Damgr_Actions actions = {};
  if (damgr_get_actions(&actions, &old_config, &config) != EXIT_SUCCESS) {
    goto cleanup;
  }

  ret = EXIT_SUCCESS;

cleanup:
  // free_config(old_config);
  // free_config(config);
  return ret;
}
