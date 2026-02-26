#include "damgr/actions.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <string.h>

static void compute_darray_diff(struct darray *negative,
                                struct darray *positive,
                                struct darray old_array,
                                struct darray new_array) {
  qsort(old_array.items, old_array.count, sizeof(old_array.items[0]), qcharcmp);
  qsort(new_array.items, new_array.count, sizeof(new_array.items[0]), qcharcmp);
  size_t i = 0;
  size_t j = 0;
  while (i < old_array.count && j < new_array.count) {
    int ret = strcmp(old_array.items[i], new_array.items[j]);
    if (ret < 0) {
      if (negative != nullptr) {
        darray_append(negative, old_array.items[i]);
      }
      ++i;
    } else if (ret > 0) {
      darray_append(positive, new_array.items[j]);
      ++j;
    } else {
      ++i;
      ++j;
    }
  }
  while (i < old_array.count) {
    if (negative != nullptr) {
      darray_append(negative, old_array.items[i]);
    }
    ++i;
  }
  while (j < new_array.count) {
    darray_append(positive, new_array.items[j]);
    ++j;
  }
}

static void actions_append(struct actions *actions, struct action action) {
  if (actions->count >= actions->capacity) {
    if (actions->capacity == 0) {
      actions->capacity = 16;
    } else {
      actions->capacity *= 2;
    }
    actions->items =
        realloc(actions->items, actions->capacity * sizeof(*actions->items));
  }
  actions->items[actions->count++] = action;
}

static void get_actions(struct actions *actions, char *name,
                        enum action_type type, bool is_positive) {
  struct action action = {.name = name,
                          .type = type,
                          .is_positive = is_positive,
                          .status = PENDING};
  actions_append(actions, action);
}

int get_actions_from_services_diff(struct actions *actions,
                                   struct darray old_services,
                                   struct darray services, bool root) {
  struct darray to_disable = {};
  struct darray to_enable = {};
  compute_darray_diff(&to_disable, &to_enable, old_services, services);
  for (size_t i = 0; i < to_disable.count; ++i) {
    char *service = string_copy(to_disable.items[i]);
    if (service == nullptr) {
      EXIT_FAILURE;
    }
    if (root) {
      get_actions(actions, service, ROOT_SERVICE, false);
    } else {
      get_actions(actions, service, USER_SERVICE, false);
    }
  }
  for (size_t i = 0; i < to_enable.count; ++i) {
    char *service = string_copy(to_enable.items[i]);
    if (service == nullptr) {
      EXIT_FAILURE;
    }
    if (root) {
      get_actions(actions, service, ROOT_SERVICE, true);
    } else {
      get_actions(actions, service, USER_SERVICE, true);
    }
  }
  return EXIT_SUCCESS;
}

static int get_actions_from_hooks_diff(struct actions *actions,
                                       struct darray old_hooks,
                                       struct darray hooks, bool root,
                                       bool pre) {
  // hooks can't be undone so no to_undo
  struct darray to_do = {};
  compute_darray_diff(nullptr, &to_do, old_hooks, hooks);
  for (size_t i = 0; i < to_do.count; ++i) {
    char *hook = string_copy(to_do.items[i]);
    if (hook == nullptr) {
      EXIT_FAILURE;
    }
    if (root) {
      if (pre) {
        get_actions(actions, hook, PRE_ROOT_HOOK, true);
      } else {
        get_actions(actions, hook, POST_ROOT_HOOK, true);
      }
    } else {
      if (pre) {
        get_actions(actions, hook, PRE_USER_HOOK, true);
      } else {
        get_actions(actions, hook, POST_USER_HOOK, true);
      }
    }
  }

  return EXIT_SUCCESS;
}
// TODO: finish these two blocks
// static int get_actions_from_packages_diff(struct actions *actions,
//                                           struct darray old_packages,
//                                           struct darray packages) {
//   return EXIT_SUCCESS;
// }
// static int get_actions_from_dotfiles_diff(struct actions *actions,
//                                           bool old_link, bool link,
//                                           char *name) {
//   return EXIT_SUCCESS;
// }

int get_actions_from_modules_diff(struct actions *actions,
                                  struct module old_module,
                                  struct module module) {
  if (get_actions_from_hooks_diff(actions, old_module.pre_root_hooks,
                                  module.pre_root_hooks, true,
                                  true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_hooks_diff(actions, old_module.pre_user_hooks,
                                  module.pre_user_hooks, false,
                                  true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  // if (get_actions_from_packages_diff(actions, old_module.packages,
  //                                    module.packages) != EXIT_SUCCESS) {
  //   return EXIT_FAILURE;
  // }
  // if (get_actions_from_packages_diff(actions, old_module.aur_packages,
  //                                    module.aur_packages) != EXIT_SUCCESS) {
  //   return EXIT_FAILURE;
  // }
  if (get_actions_from_services_diff(actions, old_module.user_services,
                                     module.user_services,
                                     false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  // if (get_actions_from_dotfiles_diff(actions, old_module.link, module.link,
  //                                    module.name) != EXIT_SUCCESS) {
  //   return EXIT_FAILURE;
  // }
  if (get_actions_from_hooks_diff(actions, old_module.post_root_hooks,
                                  module.post_root_hooks, true,
                                  false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_hooks_diff(actions, old_module.post_user_hooks,
                                  module.post_user_hooks, false,
                                  false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
