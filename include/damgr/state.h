#ifndef STATE_H
#define STATE_H
#include <stdio.h>
#include <stdlib.h>

struct darray {
  char **items;
  size_t capacity;
  size_t count;
};
void darray_append(struct darray *array, char *item);

struct module {
  struct darray pre_root_hooks;
  struct darray pre_user_hooks;
  struct darray packages;
  struct darray aur_packages;
  struct darray user_services;
  struct darray post_root_hooks;
  struct darray post_user_hooks;
  char *name;
  bool to_link;
  union {
    bool is_orphan;
    bool is_handled;
  } boolean;
};
struct modules {
  struct module *items;
  size_t capacity;
  size_t count;
};
void modules_append(struct modules *modules, struct module module);

struct host {
  struct modules modules;
  struct darray root_services;
  char *name;
};

struct config {
  struct host active_host;
  char *aur_helper;
};

int read_config(struct config *config, bool is_state);
int write_config(struct config config);

int read_host(struct host *host, bool is_state);
int write_host(struct host host);

int read_module(struct module *module, bool is_state);
int write_module(struct module module);

#endif /* STATE_H */
