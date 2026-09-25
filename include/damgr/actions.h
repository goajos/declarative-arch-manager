#ifndef ACTIONS_H
#define ACTIONS_H
#include "state.h"

enum action_status { PENDING, SUCCEEDED, FAILED };
[[maybe_unused]] static const char *action_status_names[] = {
    [PENDING] = "PENDING", [SUCCEEDED] = "SUCCEEDED", [FAILED] = "FAILED"};

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
[[maybe_unused]] static const char *action_type_names[] = {[ROOT_SERVICE] = "ROOT_SERVICE",
                                          [PRE_ROOT_HOOK] = "PRE_ROOT_HOOK",
                                          [PRE_USER_HOOK] = "PRE_USER_HOOK",
                                          [PACKAGE] = "PACKAGE",
                                          [AUR_PACKAGE] = "AUR_PACKAGE",
                                          [USER_SERVICE] = "USER_SERVICE",
                                          [DOTFILE] = "DOTFILE",
                                          [POST_ROOT_HOOK] = "POST_ROOT_HOOK",
                                          [POST_USER_HOOK] = "POST_USER_HOOK"};

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
int do_actions(struct config *old_config, struct config *config);

#endif /* ACTIONS_H */
