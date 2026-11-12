#define DAMGR_ARENA_IMPLEMENTATION
#include "damgr/arena.h"

#define DAMGR_LOG_IMPLEMENTATION
#include "damgr/log.h"

#define DAMGR_RESULT_IMPLEMENTATION
#include "damgr/result.h"

#define DAMGR_PATH_IMPLEMENTATION
#include "damgr/path.h"

int main(int argc, char *argv[]) {
  damgr_log(INFO, "damgr started!");
  if (argc == 1) {
    damgr_log(ERROR, "No damgr argument given, possible commands "
                     "are:\n\tdamgr merge\n\tdamgr commit");
    return EXIT_FAILURE;
  }
  if (argc > 2) {
    damgr_log(ERROR, "Too many damgr arguments, possible commands "
                     "are:\n\tdamgr merge\n\tdamgr commit");
    return EXIT_FAILURE;
  }

  int command_idx;
  if (strcmp(argv[1], "merge") == 0) {
    command_idx = 0;
  } else if (strcmp(argv[1], "commit") == 0) {
    command_idx = 1;
  } else {
    damgr_log(ERROR, "Not a valid damgr argument, possible commands "
                     "are:\n\tdamgr merge\n\tdamgr commit");
    return EXIT_FAILURE;
  }

  // ensure that getenv("HOME") is available for every command
  const char *home = getenv("HOME");
  if (home == nullptr) {
    damgr_log(ERROR, "getenv(\"HOME\") returns a nullptr");
    return EXIT_FAILURE;
  }

  // ensure damgr is not run as root
  if (geteuid() == 0) {
    damgr_log(ERROR, "do not run damgr as root");
    return EXIT_FAILURE;
  }

  // TODO early precondition check for early failure for commit?

  Result_t r;
  Damgr_Paths paths = {0};
  r = damgr_init_paths_result(&paths, home);
  if (!r.is_success) {
    damgr_log(ERROR, "failed to init paths:");
    damgr_err_print();
    return EXIT_FAILURE;
  }

  Damgr_Arena *arena = calloc(1, sizeof(Damgr_Arena));
  if (arena == nullptr) {
    damgr_log(ERROR, "calloc arena failed");
    return EXIT_FAILURE;
  }
  damgr_arena_init(arena);

  switch (command_idx) {
  case 0:
    damgr_log(INFO, "starting damgr merge!");
    break;
  case 1:
    damgr_log(INFO, "starting damgr commit!");
    break;
  }

  damgr_err_free();
  free(arena);
  return EXIT_SUCCESS;
}
