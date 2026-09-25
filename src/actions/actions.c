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

// TODO: use union for actions?
static void get_actions(struct actions *actions, char *name,
                        enum action_type type, bool is_positive) {
  struct action action = {.payload.name = name,
                          .type = type,
                          .is_positive = is_positive,
                          .status = PENDING};
  actions_append(actions, action);
}

static void get_packages_actions(struct actions *actions,
                                 struct darray packages, enum action_type type,
                                 bool is_positive) {
  struct action action = {.payload.packages = packages,
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
static int get_actions_from_packages_diff(struct actions *actions,
                                          struct darray old_packages,
                                          struct darray packages, bool aur) {
  struct darray to_remove = {};
  struct darray to_install = {};
  compute_darray_diff(&to_remove, &to_install, old_packages, packages);
  // for (size_t i = 0; i < to_remove.count; ++i) {
  //   char *package = string_copy(to_remove.items[i]);
  //   if (package == nullptr) {
  //     EXIT_FAILURE;
  //   }
  //   if (aur) {
  //     get_actions(actions, package, AUR_PACKAGE, false);
  //   } else {
  //     get_actions(actions, package, PACKAGE, false);
  //   }
  // }
  if (aur && to_remove.count > 0) {
    get_packages_actions(actions, to_remove, AUR_PACKAGE, false);
  } else {
    get_packages_actions(actions, to_remove, PACKAGE, false);
  }
  // for (size_t i = 0; i < to_install.count; ++i) {
  //   char *package = string_copy(to_install.items[i]);
  //   if (package == nullptr) {
  //     EXIT_FAILURE;
  //   }
  //   if (aur) {
  //     get_actions(actions, package, AUR_PACKAGE, true);
  //   } else {
  //     get_actions(actions, package, PACKAGE, true);
  //   }
  // }
  if (aur && to_install.count > 0) {
    get_packages_actions(actions, to_install, AUR_PACKAGE, true);
  } else {
    get_packages_actions(actions, to_install, PACKAGE, true);
  }
  return EXIT_SUCCESS;
}

static int get_actions_from_dotfiles_diff(struct actions *actions,
                                          bool old_link, bool link,
                                          char *name) {
  if (old_link) {
    if (!link) {
      get_actions(actions, name, DOTFILE, false);
    }
  } else {
    if (link) {
      get_actions(actions, name, DOTFILE, true);
    }
  }
  return EXIT_SUCCESS;
}

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
  if (get_actions_from_packages_diff(actions, old_module.packages,
                                     module.packages, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_packages_diff(actions, old_module.aur_packages,
                                     module.aur_packages,
                                     true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_services_diff(actions, old_module.user_services,
                                     module.user_services,
                                     false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  // no failure for dotfiles
  get_actions_from_dotfiles_diff(actions, old_module.to_link, module.to_link,
                                 module.name);

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

int get_actions_from_module(struct actions *actions, struct module module,
                            bool is_positive) {
  // can't undo hooks
  if (is_positive) {
    for (size_t i = 0; i < module.pre_root_hooks.count; ++i) {
      char *hook = string_copy(module.pre_root_hooks.items[i]);
      if (hook == nullptr) {
        EXIT_FAILURE;
      }
      get_actions(actions, hook, PRE_ROOT_HOOK, is_positive);
    }
    for (size_t i = 0; i < module.pre_user_hooks.count; ++i) {
      char *hook = string_copy(module.pre_user_hooks.items[i]);
      if (hook == nullptr) {
        EXIT_FAILURE;
      }
      get_actions(actions, hook, PRE_USER_HOOK, is_positive);
    }
  }
  // for (size_t i = 0; i < module.packages.count; ++i) {
  //   char *package = string_copy(module.packages.items[i]);
  //   if (package == nullptr) {
  //     EXIT_FAILURE;
  //   }
  //   get_actions(actions, package, PACKAGE, is_positive);
  // }
  // for (size_t i = 0; i < module.aur_packages.count; ++i) {
  //   char *package = string_copy(module.aur_packages.items[i]);
  //   if (package == nullptr) {
  //     EXIT_FAILURE;
  //   }
  //   get_actions(actions, package, AUR_PACKAGE, is_positive);
  // }
  // TODO: how is it possible that i do not have to copy the package strings
  // here? do i really have to copy all the strings when parsing then?
  get_packages_actions(actions, module.packages, PACKAGE, is_positive);
  get_packages_actions(actions, module.aur_packages, PACKAGE, is_positive);
  for (size_t i = 0; i < module.user_services.count; ++i) {
    char *service = string_copy(module.user_services.items[i]);
    if (service == nullptr) {
      EXIT_FAILURE;
    }
    get_actions(actions, service, USER_SERVICE, is_positive);
  }
  if (module.to_link) {
    get_actions(actions, module.name, DOTFILE, is_positive);
  }
  if (is_positive) {
    for (size_t i = 0; i < module.post_root_hooks.count; ++i) {
      char *hook = string_copy(module.post_root_hooks.items[i]);
      if (hook == nullptr) {
        EXIT_FAILURE;
      }
      get_actions(actions, hook, POST_ROOT_HOOK, is_positive);
    }
    for (size_t i = 0; i < module.post_user_hooks.count; ++i) {
      char *hook = string_copy(module.post_user_hooks.items[i]);
      if (hook == nullptr) {
        EXIT_FAILURE;
      }
      get_actions(actions, hook, POST_USER_HOOK, is_positive);
    }
  }
  return EXIT_SUCCESS;
}
