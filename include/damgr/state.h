#ifndef DAMGR_STATE_H
#define DAMGR_STATE_H
#include "darray.h"
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
  // TODO: union?
  char *module_name;
  char *state_path;
  char *path;
  bool is_state;
} Damgr_Module;
Damgr_Module *module_constructor(char *module_name, bool is_state);
void damgr_free_module(Damgr_Module *module);

typedef struct host {
  Damgr_Darray task_queues;
  Damgr_Darray modules;
  char *host_name;
  // TODO: union?
  char *state_path;
  char *path;
  bool is_state;
} Damgr_Host;
Damgr_Host *host_construcor(char *host_name, bool is_state);

typedef struct config {
  Damgr_Host *active_host;
  char *aur_helper;
  // TODO: union?
  char *state_path;
  char *path;
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

// void damgr_write_module(Damgr_Module module);
// void damgr_remove_module(Damgr_Module module);
//
// void damgr_write_host(Damgr_Host host);
// void damgr_remove_host(Damgr_Host host);

#endif /* DAMGR_STATE_H */
