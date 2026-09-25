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
  char *name;
  bool link;
  struct darray pre_root_hooks;
  struct darray pre_user_hooks;
  struct darray packages;
  struct darray aur_packages;
  struct darray user_services;
  struct darray post_root_hooks;
  struct darray post_user_hooks;
};
struct modules {
  struct module *items;
  size_t capacity;
  size_t count;
};
void modules_append(struct modules *modules, struct module module);

struct host {
  char *name;
  struct modules modules;
  struct darray root_services;
};

struct config {
  char *aur_helper;
  struct host active_host;
};

int read_config(struct config *config, bool is_state);
int parse_config(FILE *fid, struct config *config);
// int write_config();

int read_host(struct host *host, bool is_state);
int parse_host(FILE *fid, struct host *host);
// int write_host();

int read_module(struct module *module, bool is_state);
int parse_module(FILE *fid, struct module *module);
// int write_module();

#endif /* STATE_H */
