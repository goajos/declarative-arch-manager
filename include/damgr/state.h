#ifndef DAMGR_STATE_H
#define DAMGR_STATE_H
#include <stdlib.h>
#include <string.h>

#define DAMGR_BUFFER_SIZE 1024 * 1024 // size of the arena
#define DAMGR_ALLOC_UNIT 16           // minimal allocation unit
#define DAMGR_ARRAY_BUFFER 512        // report this max array buffer size?
#define DAMGR_PATH_MAX 4096
#define DAMGR_PRINT_MAX 1024

typedef struct arena {
  uint8_t buffer[DAMGR_BUFFER_SIZE];
  size_t offset;
} Damgr_Arena;

void damgr_arena_init(Damgr_Arena *arena);

void *damgr_arena_alloc(Damgr_Arena *arena, size_t size);

char *damgr_arena_strdup(Damgr_Arena *arena, const char *src);

typedef struct module {
  const char *module_name;
  char **packages;
  size_t packages_cnt;
  char **aur_packages;
  size_t aur_packages_cnt;
  char **root_services;
  size_t root_services_cnt;
  char **user_services;
  size_t user_services_cnt;
  bool to_link;
} Damgr_Module;

typedef struct host {
  const char *host_name;
  Damgr_Module **modules;
  size_t modules_cnt;
} Damgr_Host;

typedef struct config {
  const char *aur_helper;
  Damgr_Host *active_host;
  Damgr_Host *state_host;
} Damgr_Config;

int damgr_count_state_dir(const char *dir);

int damgr_read_state(Damgr_Config *config, Damgr_Arena *arena);
int damgr_read_active(Damgr_Config *config, Damgr_Arena *arena);

#endif /* DAMGR_STATE_H */
