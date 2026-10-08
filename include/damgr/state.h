#ifndef DAMGR_STATE_H
#define DAMGR_STATE_H
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024 * 1024 // size of the arena
#define ALLOC_UNIT 16           // minimal allocation unit

typedef struct arena {
  uint8_t buffer[BUFFER_SIZE];
  size_t offset;
} Damgr_Arena;

static inline void arena_init(Damgr_Arena *arena) {
  if (arena != nullptr)
    arena->offset = 0;
}

static inline void *arena_alloc(Damgr_Arena *arena, size_t size) {
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

static inline char *arena_strdup(Damgr_Arena *arena, const char *src) {
  if (arena == nullptr || src == nullptr)
    return nullptr;

  size_t len = strlen(src) + 1; // null terminator
  char *dest = arena_alloc(arena, len);
  if (dest != nullptr) {
    memcpy(dest, src, len);
  }

  return dest;
}

typedef struct module {
  char *module_name;
  char **packages;
  char **aur_packages;
  char **root_services;
  char **user_services;
  bool to_link;
} Damgr_Module;

typedef struct host {
  char *host_name;
  Damgr_Module *modules;
} Damgr_Host;

typedef struct config {
  char *aur_helper;
  Damgr_Host active_host;
} Damgr_Config;

#endif /* DAMGR_STATE_H */
