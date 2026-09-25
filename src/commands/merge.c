#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <stdlib.h>
#include <string.h>

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
    for (size_t i = 0; i < old_config.active_host.modules.count; ++i) {
      struct module module = old_config.active_host.modules.items[i];
      if (get_module(&module, true) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
    }
  }
  if (get_host(&new_config.active_host, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < new_config.active_host.modules.count; ++i) {
    struct module module = new_config.active_host.modules.items[i];
    if (get_module(&module, false) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
    for (size_t j = 0; j < old_config.active_host.modules.count; ++j) {
      struct module old_module = old_config.active_host.modules.items[j];
      // first check if the name lengths are equal, if so perform needle in
      // haystack search, else skip
      if (strlen(old_module.name) == strlen(module.name) &&
          string_contains(old_module.name, module.name)) {
        // old and new module available
        // get_modules_diff(old_module, module);
        // get_diff_actions();
        // do_actions();
      } else {
        // only new module available
        // get_module_actions();
        // do_actions();
      }
    }
    // TODO: how to handle the orphan old modules?
    // for every orphan -> get_module_actions -> do_actions
  }

  return EXIT_SUCCESS;
}
