#include "damgr/state.h"
#include <ctype.h>
#include <string.h>

void arena_init(Damgr_Arena *arena) {
  if (arena != nullptr)
    arena->offset = 0;
}

void *arena_alloc(Damgr_Arena *arena, size_t size) {
  if (arena == nullptr || size == 0)
    return nullptr;

  size = (size + ALLOC_UNIT - 1) & ~(ALLOC_UNIT - 1);
  if (arena->offset + size > BUFFER_SIZE) {
    return nullptr; // out of memory
  } else {
    void *ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    return ptr;
  }
}

char *arena_strdup(Damgr_Arena *arena, const char *src) {
  if (arena == nullptr || src == nullptr)
    return nullptr;

  size_t len = strlen(src) + 1; // null terminator
  char *dest = arena_alloc(arena, len);
  if (dest != nullptr) {
    memcpy(dest, src, len);
  }

  return dest;
}

[[maybe_unused]] static void damgr_trim_string(char *str) {
  if (*str == '\0' || *str == '\n')
    return;

  char *start = str;
  while (isspace((unsigned char)*start)) {
    ++start;
  }

  char *end = str + strlen(str) - 1;
  while (end > start && isspace((unsigned char)*end)) {
    --end;
  }

  *(end + 1) = '\0'; // ensure proper null termination

  if (start != str) {
    memmove(str, start, end - start + 2); // +2 includes the null terminator
  }
}

// void damgr_read_config(Damgr_Config *config, Damgr_Arena *arena,
//                        const char *fp) {}
// void damgr_read_host(Damgr_Host *host, Damgr_Arena *arena, const char *fp) {}
// void damgr_read_module(Damgr_Module *module, Damgr_Arena *arena,
//                        const char *fp) {}
