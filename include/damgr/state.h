#ifndef DAMGR_STATE_H
#define DAMGR_STATE_H
#include <stdio.h>
#include <stdlib.h>

typedef struct darray {
  char **items;
  size_t capacity;
  size_t count;
} Damgr_Darray;
void damgr_free_darray(Damgr_Darray *darray);
void damgr_darray_append(Damgr_Darray *darray, char *item);

typedef struct module {
  Damgr_Darray pre_root_hooks;
  Damgr_Darray pre_user_hooks;
  Damgr_Darray packages;
  Damgr_Darray aur_packages;
  Damgr_Darray user_services;
  Damgr_Darray post_root_hooks;
  Damgr_Darray post_user_hooks;
  char *name;
  bool to_link;
  union {
    bool is_orphan;
    bool is_compared;
  };
  bool to_write; // flags that a queue with this module ran successfully
} Damgr_Module;

typedef struct modules {
  Damgr_Module *items;
  size_t capacity;
  size_t count;
} Damgr_Modules;
void damgr_modules_append(Damgr_Modules *modules, Damgr_Module module);

typedef struct root_servives {
  char **items;
  size_t capacity;
  size_t count;
  bool to_write; // flags that a queue with these root services ran successfully
} Damgr_Root_Services;
void damgr_root_services_append(Damgr_Root_Services *services, char *service);

typedef struct host {
  Damgr_Modules modules;
  Damgr_Root_Services root_services;
  char *name;
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

void damgr_free_config(Damgr_Config *config);
int damgr_read_config(char *user, Damgr_Config *config, bool is_state);
int damgr_write_config(char *user, Damgr_Config config);
int damgr_read_host(char *user, Damgr_Config *config, bool is_state);
int damgr_read_module(char *user, Damgr_Config *config, int module_idx,
                      bool is_state);

#endif /* DAMGR_STATE_H */
