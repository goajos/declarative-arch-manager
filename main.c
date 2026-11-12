#define DAMGR_ARENA_IMPLEMENTATION
#include "damgr/arena.h"

#define DAMGR_LOG_IMPLEMENTATION
#include "damgr/log.h"

#define DAMGR_RESULT_IMPLEMENTATION
#include "damgr/result.h"

#define DAMGR_UTILS_IMPLEMENTATION
#include "damgr/utils.h"

#define DAMGR_PATH_IMPLEMENTATION
#include "damgr/path.h"

#define DAMGR_CONTEXT_IMPLEMENTATION
#include "damgr/context.h"

#define DAMGR_JOURNAL_IMPLEMENTATION
#include "damgr/journal.h"

#define DAMGR_COMMIT_IMPLEMENTATION
#include "damgr/commit.h"

#define DAMGR_MERGE_IMPLEMENTATION
#include "damgr/merge.h"

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

  Damgr_Paths paths = {0};
  if (!(damgr_init_paths_result(&paths, home)).is_success) {
    damgr_log(ERROR, "failed to init paths:");
    damgr_err_print();
    damgr_err_free();
    return EXIT_FAILURE;
  }

  bool exists;
  if (!(damgr_make_dir_result(paths.state_dir, &exists)).is_success) {
    damgr_log(ERROR, "state folder should exist: %s", paths.state_dir);
    damgr_err_print();
    damgr_err_free();
    return EXIT_FAILURE;
  }

  // command_idx == 0: damgr merge
  if (command_idx == 0 &&
      !(damgr_merge_preconditions_result(&paths)).is_success) {
    damgr_log(ERROR, "merge preconditions failed:");
    damgr_err_print();
    damgr_err_free();
    return EXIT_FAILURE;
  }

  // command_idx == 1: damgr commit
  if (command_idx == 1 &&
      !(damgr_commit_preconditions_result(&paths)).is_success) {
    damgr_log(ERROR, "commit preconditions failed:");
    damgr_err_print();
    damgr_err_free();
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
    Damgr_Context context = {0};
    if (!(damgr_read_context_result(&paths, &context, arena)).is_success) {
      damgr_log(ERROR, "failed to read context:");
      damgr_err_print();
      damgr_err_free();
      return EXIT_FAILURE;
    }
    break;
  case 1:
    damgr_log(INFO, "starting damgr commit!");
    break;
  }

  free(arena);
  return EXIT_SUCCESS;
}
