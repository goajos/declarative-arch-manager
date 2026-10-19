#include "damgr/arena.h"
#include "damgr/commit.h"
#include "damgr/log.h"
#include "damgr/merge.h"
#include "damgr/paths.h"
#include "damgr/stage.h"

int main(int argc, char *argv[]) {
  damgr_log(INFO, "damgr started!");
  if (argc == 1) {
    damgr_log(
        ERROR,
        "No damgr argument given, possible commands "
        "are:\n\tdamgr validate\n\tdamgr merge\n\tdamgr stage\n\tdamgr commit");
    return EXIT_FAILURE;
  }
  if (argc > 2) {
    damgr_log(
        ERROR,
        "Too many damgr arguments, possible commands "
        "are:\n\tdamgr validate\n\tdamgr merge\n\tdamgr stage\n\tdamgr commit");
    return EXIT_FAILURE;
  }

  int command_idx;
  if (strcmp(argv[1], "validate") == 0) {
    command_idx = 0;
  } else if (strcmp(argv[1], "merge") == 0) {
    command_idx = 1;
  } else if (strcmp(argv[1], "stage") == 0) {
    command_idx = 2;
  } else if (strcmp(argv[1], "commit") == 0) {
    command_idx = 3;
  } else {
    damgr_log(
        ERROR,
        "Not a valid damgr argument, possible commands "
        "are:\n\tdamgr validate\n\tdamgr merge\n\tdamgr stage\n\tdamgr commit");
    return EXIT_FAILURE;
  }

  // ensure that getenv("HOME") is available for every command
  const char *home = getenv("HOME");
  if (home == nullptr) {
    damgr_log(ERROR, "getenv(\"HOME\") returns a nullptr");
    return EXIT_FAILURE;
  }

  // Damgr_Paths paths = {nullptr};
  Damgr_Paths paths = {0};
  damgr_paths_init(&paths, home);

  Damgr_Arena *arena = calloc(1, sizeof(Damgr_Arena));
  if (arena == nullptr)
    return EXIT_FAILURE;
  damgr_arena_init(arena);

  int ret;
  switch (command_idx) {
  case 0:
    damgr_log(INFO, "starting damgr validate!");
    // ret = damgr_validate();
    ret = 1;
    break;
  case 1:
    damgr_log(INFO, "starting damgr merge!");
    ret = damgr_merge(&paths, arena);
    break;
  case 2:
    damgr_log(INFO, "starting damgr stage!");
    ret = damgr_stage(&paths, arena);
    break;
  case 3:
    damgr_log(INFO, "starting damgr commit!");
    ret = damgr_commit(&paths);
    break;
  }

  free(arena);
  if (ret != EXIT_SUCCESS) {
    damgr_log(ERROR, "damgr %s failed...", argv[1]);
  } else {
    damgr_log(INFO, "damgr finished!");
  }
  return ret;
}
