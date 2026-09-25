#include "damgr/actions.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <stdlib.h>
#include <string.h>

int damgr_merge() {
  LOG(LOG_INFO, "running damgr merge...");

  int ret;
  char fidbuf[PATH_MAX];
  struct config old_config = {};
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr", get_user());
  ret = is_damgr_state_dir_empty(fidbuf);
  if (is_damgr_state_dir_empty(fidbuf) == EXIT_FAILURE) {
    LOG(LOG_ERROR, "failed to open state directory: %s", fidbuf);
  }
  if (ret == 2) { // only "." and ".." dirs found
    LOG(LOG_INFO, "no state to parse, the state directory is empty: %s",
        fidbuf);
  } else {
    if (read_config(&old_config, true) != EXIT_SUCCESS) {
      goto cleanup;
    };
  }
  ret = EXIT_FAILURE;
  struct config config = {};
  if (read_config(&config, false) != EXIT_SUCCESS) {
    goto cleanup;
  };

  if (old_config.active_host.name != nullptr) {
    if (read_host(&old_config.active_host, true) != EXIT_SUCCESS) {
      goto cleanup;
    }
    for (size_t i = 0; i < old_config.active_host.modules.count; ++i) {
      struct module *module = &old_config.active_host.modules.items[i];
      module->boolean.is_orphan = true;
      if (read_module(module, true) != EXIT_SUCCESS) {
        goto cleanup;
      }
    }
  }
  if (read_host(&config.active_host, false) != EXIT_SUCCESS) {
    goto cleanup;
  }
  for (size_t i = 0; i < config.active_host.modules.count; ++i) {
    struct module *module = &config.active_host.modules.items[i];
    module->boolean.is_done = false;
    if (read_module(module, false) != EXIT_SUCCESS) {
      goto cleanup;
    }
  }

  if (get_actions(&old_config, &config) != EXIT_SUCCESS) {
    goto cleanup;
  }

  if (old_config.active_host.host_actions.count == 0 &&
      old_config.active_host.modules_actions_count == 0 &&
      config.active_host.host_actions.count == 0 &&
      config.active_host.modules_actions_count == 0) {
    LOG(LOG_INFO, "got no actions, nothing to do...");
  } else {
    if (do_actions(&old_config, &config) != EXIT_SUCCESS) {
      goto cleanup;
    }

    write_config(&config);
    write_host(&config.active_host);
    for (size_t i = 0; i < config.active_host.modules.count; ++i) {
      struct module module = config.active_host.modules.items[i];
      if (module.boolean.is_done) {
        write_module(&module);
      }
    }
  }

  ret = EXIT_SUCCESS;

cleanup:
  if (old_config.aur_helper != nullptr) {
    free_config(old_config);
  }
  free_config(config);
  return ret;
}
