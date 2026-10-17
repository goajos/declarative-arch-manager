#ifndef DAMGR_CONTEXT_H
#define DAMGR_CONTEXT_H
#include "damgr/arena.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DAMGR_ARRAY_BUFFER_MAX 512
#define DAMGR_LINE_MAX 512

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

typedef struct context {
  const char *aur_helper;
  Damgr_Host *active_host;
  Damgr_Host *state_host;
} Damgr_Context;

int damgr_read_state_context(Damgr_Context *context, Damgr_Arena *arena);
int damgr_read_active_context(Damgr_Context *context, Damgr_Arena *arena);

#endif /* DAMGR_CONTEXT_H */
