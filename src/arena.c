#include "damgr/arena.h"
#include <string.h>

void damgr_arena_init(Damgr_Arena *arena) {
  if (arena != nullptr)
    arena->offset = 0;
}

void *damgr_arena_alloc(Damgr_Arena *arena, size_t size) {
  if (arena == nullptr || size == 0)
    return nullptr;

  size = (size + DAMGR_ALLOC_UNIT - 1) & ~(DAMGR_ALLOC_UNIT - 1);
  if (arena->offset + size > DAMGR_BUFFER_SIZE) {
    return nullptr; // out of memory
  } else {
    void *ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    memset(ptr, 0, size); // zero memory
    return ptr;
  }
}

char *damgr_arena_strdup(Damgr_Arena *arena, const char *src) {
  if (arena == nullptr || src == nullptr)
    return nullptr;

  size_t len = strlen(src) + 1; // null terminator
  char *dest = damgr_arena_alloc(arena, len);
  if (dest != nullptr) {
    memcpy(dest, src, len);
  }

  return dest;
}
