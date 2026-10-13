#include "damgr/diff.h"
#include "damgr/log.h"
#include "damgr/state.h"

int damgr_merge() {
  Damgr_Arena *arena = calloc(1, sizeof(Damgr_Arena));
  if (arena == nullptr)
    return EXIT_FAILURE;
  arena_init(arena);

  Damgr_Config config = {};

  int ret;
  char fidbuf[DAMGR_PATH_MAX];
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr", getenv("HOME"));
  ret = damgr_count_state_dir(fidbuf);
  if (ret == EXIT_FAILURE) {
    return ret;
  } else if (ret == 2) { // only "." and ".." dirs found
    damgr_log(INFO, "no state to parse, the state directory is empty: %s",
              fidbuf);
  } else {
    ret = damgr_read_state(&config, arena);
    if (ret == EXIT_FAILURE)
      return ret;
  }
  ret = damgr_read_active(&config, arena);
  if (ret == EXIT_FAILURE)
    return ret;

  Damgr_Host_Diff host_diff = {};
  // TODO: add a return here?
  damgr_get_diff_host(&host_diff, config.state_host, config.active_host, arena);

  free(arena);
  return EXIT_SUCCESS;
}
