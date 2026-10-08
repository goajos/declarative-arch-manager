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

void arena_init(Damgr_Arena *arena);

void *arena_alloc(Damgr_Arena *arena, size_t size);

char *arena_strdup(Damgr_Arena *arena, const char *src);

// TODO: what to do with the (state_)path for modules?
typedef struct module {
  char *module_name;
  char **packages;
  char **aur_packages;
  char **root_services;
  char **user_services;
  bool to_link;
} Damgr_Module;

// TODO: what to do with the (state_)path for hosts?
typedef struct host {
  char *host_name;
  Damgr_Module *modules;
} Damgr_Host;

// TODO: what to do with the (state_)path for config?
typedef struct config {
  char *aur_helper;
  Damgr_Host active_host;
} Damgr_Config;

void damgr_read_config(Damgr_Config *config, Damgr_Arena *arena,
                       const char *fp);
void damgr_read_host(Damgr_Host *host, Damgr_Arena *arena, const char *fp);
void damgr_read_module(Damgr_Module *module, Damgr_Arena *arena,
                       const char *fp);

#endif /* DAMGR_STATE_H */
