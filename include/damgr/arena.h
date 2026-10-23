#ifndef DAMGR_ARENA_H
#define DAMGR_ARENA_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

constexpr size_t ARENA_BUFFER_SIZE = 1024 * 1024;
constexpr size_t ALIGN = 16;

typedef struct arena {
  uint8_t arena_buffer[ARENA_BUFFER_SIZE];
  size_t offset;
} Damgr_Arena;

void damgr_arena_init(Damgr_Arena *arena);
void *damgr_arena_alloc(Damgr_Arena *arena, size_t size);
char *damgr_arena_strdup(Damgr_Arena *arena, const char *src);

#ifdef DAMGR_ARENA_IMPLEMENTATION

void damgr_arena_init(Damgr_Arena *arena) {
  if (arena != nullptr)
    arena->offset = 0;
}

void *damgr_arena_alloc(Damgr_Arena *arena, size_t size) {
  if (arena == nullptr || size == 0)
    return nullptr;

  size = (size + ALIGN - 1) & ~(ALIGN - 1);
  if (arena->offset + size > ARENA_BUFFER_SIZE) {
    return nullptr; // out of memory
  } else {
    void *ptr = &arena->arena_buffer[arena->offset];
    arena->offset += size;
    memset(ptr, 0, size); // zero memory
    return ptr;
  }
}

char *damgr_arena_strdup(Damgr_Arena *arena, const char *src) {
  if (arena == nullptr || src == nullptr)
    return nullptr;

  size_t len = strlen(src) + 1; // null terminator
  char *dst = damgr_arena_alloc(arena, len);
  if (dst != nullptr) {
    memcpy(dst, src, len);
  }

  return dst;
}

#endif /* DAMGR_ARENA_IMPLEMENTATION */
#endif /* DAMGR_ARENA_H */
