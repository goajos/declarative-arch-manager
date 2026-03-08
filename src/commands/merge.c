#include "damgr/actions.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <stdlib.h>
#include <string.h>

int damgr_merge() {
  damgr_log(INFO, "running damgr merge...");

  char *user = get_user();
  if (user == nullptr) {
    return EXIT_FAILURE;
  }
  int ret;
  char fidbuf[PATH_MAX];
  struct config old_config = {};
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr", user);
  ret = is_damgr_state_dir_empty(fidbuf);
  if (is_damgr_state_dir_empty(fidbuf) == EXIT_FAILURE) {
    damgr_log(ERROR, "failed to open state directory: %s", fidbuf);
  }
  if (ret == 2) { // only "." and ".." dirs found
    damgr_log(INFO, "no state to parse, the state directory is empty: %s",
              fidbuf);
  } else {
    if (read_config(user, &old_config, true) != EXIT_SUCCESS) {
      goto cleanup;
    };
  }
  ret = EXIT_FAILURE;
  struct config config = {};
  if (read_config(user, &config, false) != EXIT_SUCCESS) {
    goto cleanup;
  };

  if (old_config.active_host.name != nullptr) {
    if (read_host(user, &old_config.active_host, true) != EXIT_SUCCESS) {
      goto cleanup;
    }
    for (size_t i = 0; i < old_config.active_host.modules.count; ++i) {
      struct module *module = &old_config.active_host.modules.items[i];
      module->boolean.is_orphan = true;
      if (read_module(user, &old_config.active_host.modules.items[i], true) !=
          EXIT_SUCCESS) {
        goto cleanup;
      }
    }
  }
  if (read_host(user, &config.active_host, false) != EXIT_SUCCESS) {
    goto cleanup;
  }
  for (size_t i = 0; i < config.active_host.modules.count; ++i) {
    struct module *module = &config.active_host.modules.items[i];
    module->boolean.is_done = false;
    if (read_module(user, module, false) != EXIT_SUCCESS) {
      goto cleanup;
    }
  }

  if (get_actions(&old_config, &config) != EXIT_SUCCESS) {
    goto cleanup;
  }

  if (old_config.active_host.host_actions.count == 0 &&
      old_config.active_host.all_modules_actions_count == 0 &&
      config.active_host.host_actions.count == 0 &&
      config.active_host.all_modules_actions_count == 0) {
    damgr_log(INFO, "got no actions, nothing to do...");
  } else {
    if (do_actions(&old_config, &config) != EXIT_SUCCESS) {
      goto cleanup;
    }

    write_config(user, config);
    write_host(user, config.active_host);
    for (size_t i = 0; i < config.active_host.modules.count; ++i) {
      struct module module = config.active_host.modules.items[i];
      if (module.boolean.is_done) {
        write_module(user, module);
      } else {
        report_module_actions(module, false);
      }
    }
    for (size_t i = 0; i < old_config.active_host.modules.count; ++i) {
      struct module module = old_config.active_host.modules.items[i];
      if (!module.boolean.is_done) {
        report_module_actions(module, true);
      }
    }
  }

  ret = EXIT_SUCCESS;

cleanup:
  free_config(old_config);
  free_config(config);
  return ret;
}
