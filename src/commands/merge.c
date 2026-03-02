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
  struct config config = {};
  if (read_config(&config, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  };

  if (old_config.active_host.name != nullptr) {
    if (read_host(&old_config.active_host, true) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
    for (size_t i = 0; i < old_config.active_host.modules.count; ++i) {
      struct module *module = &old_config.active_host.modules.items[i];
      module->boolean.is_orphan = true;
      if (read_module(module, true) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
    }
  }
  if (read_host(&config.active_host, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < config.active_host.modules.count; ++i) {
    struct module *module = &config.active_host.modules.items[i];
    module->boolean.is_done = false;
    if (read_module(module, false) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }
  LOG(LOG_INFO, "successfully read all files. getting actions!");

  if (get_actions(&old_config, &config) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  puts("");
  // write_config(&config);
  // write_host(&config.active_host);
  // for (size_t i = 0; i < config.active_host.modules.count; ++i) {
  //   struct module module = config.active_host.modules.items[i];
  //   if (module.boolean.is_done) {
  //     write_module(&module);
  //   } else {
  //     LOG(LOG_ERROR, "unhandled module: %s", module.name);
  //   }
  // }

  free_config(old_config);
  free_config(config);

  return EXIT_SUCCESS;
}
