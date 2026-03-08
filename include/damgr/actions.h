#ifndef DAMGR_ACTIONS_H
#define DAMGR_ACTIONS_H
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
  char *name;
  struct darray packages;
};

struct action {
  struct payload payload;
  enum action_status status;
  enum action_type type;
  bool is_positive;
};

int get_actions(struct config *old_config, struct config *config);
int do_actions(char *user, struct config *old_config, struct config *config);

#endif /* DAMGR_ACTIONS_H */
