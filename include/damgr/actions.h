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

union payload {
  char *name;
  struct darray packages;
};

struct action {
  union payload payload;
  enum action_status status;
  enum action_type type;
  bool is_positive;
};

// TODO: or do_actions()?
int get_actions(struct config *old_config, struct config *config);
int do_action(struct action action, char *aur_helper);
// int undo_action(struct action action, char *aur_helper);

#endif /* ACTIONS_H */
