#ifndef STATE_H
#define STATE_H
#include <stdio.h>

struct darray {
  char **items;
  size_t capacity;
  size_t count;
};

struct module {
  char *name;
};

struct modules {
  struct module *items;
  size_t capacity;
  size_t count;
};

struct host {
  char *name;
  struct module modules;
  struct darray root_services;
};

struct config {
  char *aur_helper;
  struct host active_host;
};

int get_config(struct config *config, bool state);
int parse_config(FILE *fid, struct config *config);

#endif /* STATE_H */
