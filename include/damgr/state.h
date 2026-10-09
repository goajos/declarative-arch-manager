#ifndef DAMGR_STATE_H
#define DAMGR_STATE_H
#include <stdlib.h>
#include <string.h>

#define DAMGR_BUFFER_SIZE 1024 * 1024 // size of the arena
#define DAMGR_ALLOC_UNIT 16           // minimal allocation unit
#define DAMGR_PATH_MAX 4096
#define DAMGR_LIST_BUFFER 256

typedef struct arena {
  uint8_t buffer[DAMGR_BUFFER_SIZE];
  size_t offset;
} Damgr_Arena;

void arena_init(Damgr_Arena *arena);

void *arena_alloc(Damgr_Arena *arena, size_t size);

char *arena_strdup(Damgr_Arena *arena, const char *src);

// TODO: what to do with the (state_)path for modules?
typedef struct module {
  char *module_name;
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

// TODO: what to do with the (state_)path for hosts?
typedef struct host {
  char *host_name;
  Damgr_Module **modules;
  size_t modules_cnt;
} Damgr_Host;

// TODO: what to do with the (state_)path for config?
typedef struct config {
  char *aur_helper;
  Damgr_Host *active_host;
  Damgr_Host *state_host;
} Damgr_Config;

int damgr_is_state_dir_empty(const char *dir);

int damgr_read_state(Damgr_Config *config, Damgr_Arena *arena);
int damgr_read(Damgr_Config *config, Damgr_Arena *arena);

#endif /* DAMGR_STATE_H */
