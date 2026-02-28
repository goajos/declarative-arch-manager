#ifndef ACTIONS_H
#define ACTIONS_H
#include "state.h"

enum action_status { PENDING, SUCCEEDED, FAILED };

enum action_type {
  ROOT_SERVICE,
  PRE_ROOT_HOOK,
  PRE_USER_HOOK,
  PACKAGE,
  AUR_PACKAGE,
  USER_SERVICE,
  DOTFILE,
  POST_ROOT_HOOK,
  POST_USER_HOOK,
};

struct payload {
  struct darray packages;
  char *name;
};

struct action {
  struct payload payload;
  enum action_status status;
  enum action_type type;
  bool is_positive;
};

struct actions {
  struct action *items;
  size_t capacity;
  size_t count;
};

int get_actions_from_services_diff(struct actions *actions,
                                   struct darray old_services,
                                   struct darray ervices, bool is_root);
int get_actions_from_modules_diff(struct actions *actions,
                                  struct module old_module,
                                  struct module module);
int get_actions_from_module(struct actions *actions, struct module module,
                            bool is_postive);

int do_action(struct action action, char *aur_helper);
int undo_action(struct action action, char *aur_helper);

#endif /* ACTIONS_H */
