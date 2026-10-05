#ifndef DAMGR_STATE_H
#define DAMGR_STATE_H
#include "darray.h"
#include "tasks.h"
#include <stdlib.h>

typedef struct module {
  Damgr_Darray pre_root_hooks;
  Damgr_Darray pre_user_hooks;
  Damgr_Darray packages;
  Damgr_Darray aur_packages;
  Damgr_Darray user_services;
  Damgr_Darray root_services;
  Damgr_Darray post_root_hooks;
  Damgr_Darray post_user_hooks;
  bool to_link;
  char *module_name;
} Damgr_Module;

typedef struct modules {
  Damgr_Module *modules;
  size_t capacity;
  size_t count;
} Damgr_Modules;
void damgr_modules_append(Damgr_Modules *modules, Damgr_Module module);

typedef struct host {
  Damgr_Task_Queues task_queues;
  Damgr_Modules modules;
  char *host_name;
} Damgr_Host;

typedef struct config {
  Damgr_Host active_host;
  char *aur_helper;
} Damgr_Config;

typedef enum conf_key {
  AUR_HELPER,
  ACTIVE_HOST,
  MODULES,
  SERVICES,
  DOTFILES,
  PACKAGES,
  AUR_PACKAGES,
  PRE_HOOKS,
  POST_HOOKS,
  MAX_CONF_KEY,
} Damgr_Conf_Key;
extern const char *damgr_conf_keys[];

void damgr_read_config(Damgr_Config *config, bool is_state);
void damgr_read_host(Damgr_Config *config, bool is_state);
void damgr_read_module(Damgr_Config *config, int module_idx, bool is_state);

void damgr_write_module(Damgr_Module module);
void damgr_remove_module(Damgr_Module module);

void damgr_write_host(Damgr_Host host);
void damgr_remove_host(Damgr_Host host);

#endif /* DAMGR_STATE_H */
