#include "damgr/actions.h"
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
  if (is_damgr_state_dir_empty(fidbuf) == EXIT_FAILURE) {
    LOG(LOG_ERROR, "failed to open state directory: %s", fidbuf);
    return EXIT_FAILURE;
  }
  if (ret == 2) { // only "." and ".." dirs found
    LOG(LOG_INFO, "no state to parse, the state directory is empty: %s",
        fidbuf);
  } else {
    if (read_config(&old_config, true) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    };
  }
  struct config new_config = {};
  if (read_config(&new_config, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  };

  if (old_config.active_host.name != nullptr) {
    if (read_host(&old_config.active_host, true) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
    for (size_t i = 0; i < old_config.active_host.modules.count; ++i) {
      // work with pointer here so that we can use the old modules as orphans
      struct module *module = &old_config.active_host.modules.items[i];
      module->is_orphan = true;
      if (read_module(module, true) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
    }
  }
  if (read_host(&new_config.active_host, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  struct actions actions = {};
  if (get_actions_from_services_diff(
          &actions, old_config.active_host.root_services,
          new_config.active_host.root_services, true) != EXIT_SUCCESS) {
    LOG(LOG_ERROR, "failed to get actions for the host root services");
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < new_config.active_host.modules.count; ++i) {
    struct module module = new_config.active_host.modules.items[i];
    if (read_module(&module, false) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
    for (size_t j = 0; j < old_config.active_host.modules.count; ++j) {
      struct module old_module = old_config.active_host.modules.items[j];
      // first check if the name lengths are equal, if so perform needle in
      // haystack search, else skip
      if (strlen(old_module.name) == strlen(module.name) &&
          string_contains(old_module.name, module.name)) {
        // old and new module available
        old_module.is_orphan = false;
        if (get_actions_from_modules_diff(&actions, old_module, module) !=
            EXIT_SUCCESS) {
          LOG(LOG_ERROR, "failed to get actions for modules: %s %s",
              old_module.name, module.name);
          return EXIT_FAILURE;
        }
      } else {
        if (get_actions_from_module(&actions, module, true) != EXIT_SUCCESS) {
          LOG(LOG_ERROR, "failed to get actions for module: %s", module.name);
          return EXIT_FAILURE;
        }
      }
    }
  }
  for (size_t i = 0; i < old_config.active_host.modules.count; ++i) {
    struct module old_module = old_config.active_host.modules.items[i];
    if (old_module.is_orphan) {
      if (get_actions_from_module(&actions, old_module, false) !=
          EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to get actions for module: %s", old_module.name);
        return EXIT_FAILURE;
      }
    }
  }
  // TODO: handle actions
  // is success => do_action()
  // if failure => undo_action()
  printf("eureka?");

  return EXIT_SUCCESS;
}
