#include "damgr/state.h"
#include <stdlib.h>

int damgr_merge() {
  Damgr_Arena *arena = calloc(1, sizeof(Damgr_Arena));
  if (arena == nullptr)
    return EXIT_FAILURE;
  arena_init(arena);

  free(arena);
  return EXIT_SUCCESS;
}
