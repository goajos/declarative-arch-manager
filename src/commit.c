#include "damgr/commit.h"

int damgr_commit_with_context([[maybe_unused]] const Damgr_Context context) {
  // int ret;
  return EXIT_SUCCESS;
}

int damgr_commit(Damgr_Arena *arena) {
  int ret;

  Damgr_Context context = {0};
  ret = damgr_read_state_context(&context, arena);
  if (ret == EXIT_FAILURE)
    return ret;

  ret = damgr_commit_with_context(context);

  return EXIT_SUCCESS;
}
