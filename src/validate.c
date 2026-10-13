#include "damgr/log.h"
#include "damgr/state.h"

int damgr_validate() {
  Damgr_Arena *arena = calloc(1, sizeof(Damgr_Arena));
  if (arena == nullptr)
    return EXIT_FAILURE;
  arena_init(arena);

  Damgr_Config config = {};

  int ret;
  char fidbuf[DAMGR_PATH_MAX];
  if (getenv("HOME") == nullptr) {
    damgr_log(ERROR, "getenv(\"HOME\") return a nullptr");
    ret = EXIT_FAILURE;
    goto cleanup;
  }
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr", getenv("HOME"));
  ret = damgr_count_state_dir(fidbuf);
  if (ret == EXIT_FAILURE) {
    goto cleanup;
  } else if (ret == 2) { // only "." and ".." dirs found
    damgr_log(INFO, "no state to parse, the state directory is empty: %s",
              fidbuf);
  } else {
    ret = damgr_read_state(&config, arena);
    if (ret == EXIT_FAILURE)
      goto cleanup;
  }
  ret = damgr_read_active(&config, arena);
  if (ret == EXIT_FAILURE)
    goto cleanup;

  // for (size_t i = 0; i < config.active_host->modules_cnt; ++i) {
  // find_module(config.active_host->modules, config.active_host->modules_cnt,
  //             config.state_host->modules[0]->module_name);
  if (config.state_host != nullptr) {
    if (strcmp(config.state_host->host_name, config.active_host->host_name) ==
        0) {
      // active host and state host are the same
    } else {
      // active host and state host are different
    }
  } else {
    // only active host
  }
  ret = EXIT_SUCCESS;

cleanup:
  free(arena);
  return ret;
}
