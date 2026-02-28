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

// TODO: use a union for the actions?
struct action {
  enum action_type type;
  bool is_positive;
  enum action_status status;
  union {
    char *name;
    struct darray packages;
  } payload;
};

// struct action {
//   char *name;
//   enum action_type type;
//   bool is_positive;
//   enum action_status status;
// };

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

// TODO: actions can be handled sequentially?
// set status from pending to either succeeded or failed
// succeeded means go forward and do next action
// failed means go back and undo all actions?
// int do_actions();
// int undo_actions();

#endif /* ACTIONS_H */
