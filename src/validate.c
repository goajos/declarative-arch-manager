#include "damgr/log.h"
#include "damgr/state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int damgr_validate() {
  Damgr_Arena *arena = calloc(1, sizeof(Damgr_Arena));
  if (arena == nullptr)
    return EXIT_FAILURE;
  arena_init(arena);

  Damgr_Config config = {};

  int ret;
  char fidbuf[DAMGR_PATH_MAX];
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr", getenv("HOME"));
  ret = damgr_is_state_dir_empty(fidbuf);
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
  ret = damgr_read(&config, arena);
  if (ret == EXIT_FAILURE)
    return ret;
  free(arena);
  return EXIT_SUCCESS;
}
