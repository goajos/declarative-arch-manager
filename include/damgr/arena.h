#ifndef DAMGR_ARENA_H
#define DAMGR_ARENA_H
#include <stdlib.h>

#define DAMGR_BUFFER_SIZE 1024 * 1024 // size of the arena
#define DAMGR_ALLOC_UNIT 16           // minimal allocation unit

typedef struct arena {
  uint8_t buffer[DAMGR_BUFFER_SIZE];
  size_t offset;
} Damgr_Arena;

void damgr_arena_init(Damgr_Arena *arena);
void *damgr_arena_alloc(Damgr_Arena *arena, size_t size);
char *damgr_arena_strdup(Damgr_Arena *arena, const char *src);

#endif /* DAMGR_ARENA_H */
