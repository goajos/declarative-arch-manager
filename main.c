#define DAMGR_ARENA_IMPLEMENTATION
#include "damgr/arena.h"

#define DAMGR_LOG_IMPLEMENTATION
#include "damgr/log.h"

#define DAMGR_RESULT_IMPLEMENTATION
#include "damgr/result.h"

#define DAMGR_PATHS_IMPLEMENTATION
#include "damgr/paths.h"

#define DAMGR_UTILS_IMPLEMENTATION
#include "damgr/utils.h"

#define DAMGR_CONTEXT_IMPLEMENTATION
#include "damgr/context.h"

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

  Result_t r;
  Damgr_Paths paths = {0};
  r = damgr_paths_init_r(&paths, home);
  if (!r.is_success) {
    damgr_log(ERROR, "failed to init paths: %s", r.error);
    return EXIT_FAILURE;
  }

  Damgr_Arena *arena = calloc(1, sizeof(Damgr_Arena));
  if (arena == nullptr) {
    damgr_log(ERROR, "calloc arena failed");
    return EXIT_FAILURE;
  }
  damgr_arena_init(arena);

  struct stat st;
  // TODO: do we need context everytime?
  Damgr_Context context = {0};
  r = damgr_read_context_r(&paths, &context, arena, &st);
  if (!r.is_success) {
    damgr_log(ERROR, "failed to read context: %s", r.error);
    goto cleanup;
  }

  switch (command_idx) {
  case 0:
    damgr_log(INFO, "starting damgr merge!");
    // TODO: journal guard?
    r = damgr_check_merge_preconditions(&paths, &st);
    if (!r.is_success) {
      damgr_log(ERROR, "merge pre condition failed: %s", r.error);
      goto cleanup;
    }
    // ret = damgr_merge(&paths, arena);
    break;
  case 1:
    damgr_log(INFO, "starting damgr commit!");
    // TODO: journal guard?
    r = damgr_check_commit_preconditions(&paths, &st);
    if (!r.is_success) {
      damgr_log(ERROR, "commit pre condition failed: %s", r.error);
      goto cleanup;
    }
    // ret = damgr_commit(&paths);
    break;
  }

  free(arena);
  return EXIT_SUCCESS;
cleanup:
  free(arena);
  return EXIT_FAILURE;
}
