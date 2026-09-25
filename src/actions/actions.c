#include "damgr/actions.h"
#include "damgr/log.h"
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
  struct payload payload = {.name = name, .packages = {}};
  struct action action = {.payload = payload,
                          .status = PENDING,
                          .type = type,
                          .is_positive = is_positive};
  actions_append(actions, action);
}

static void get_packages_actions(struct actions *actions,
                                 struct darray packages, enum action_type type,
                                 bool is_positive, char *module_name) {
  struct payload payload = {.name = module_name, .packages = packages};
  struct action action = {.payload = payload,
                          .status = PENDING,
                          .type = type,
                          .is_positive = is_positive};
  actions_append(actions, action);
}

int get_actions_from_services_diff(struct actions *actions,
                                   struct darray old_services,
                                   struct darray services, bool root) {
  struct darray to_disable = {};
  struct darray to_enable = {};
  compute_darray_diff(&to_disable, &to_enable, old_services, services);
  for (size_t i = 0; i < to_disable.count; ++i) {
    char *service = to_disable.items[i];
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
    char *service = to_enable.items[i];
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
    char *hook = to_do.items[i];
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
                                          struct darray packages, bool aur,
                                          char *module_name) {
  struct darray to_remove = {};
  struct darray to_install = {};
  compute_darray_diff(&to_remove, &to_install, old_packages, packages);
  if (aur && to_remove.count > 0) {
    get_packages_actions(actions, to_remove, AUR_PACKAGE, false, module_name);
  } else if (!aur && to_remove.count > 0) {
    get_packages_actions(actions, to_remove, PACKAGE, false, module_name);
  }
  if (aur && to_install.count > 0) {
    get_packages_actions(actions, to_install, AUR_PACKAGE, true, module_name);
  } else if (!aur && to_install.count > 0) {
    get_packages_actions(actions, to_install, PACKAGE, true, module_name);
  }
  return EXIT_SUCCESS;
}

static int get_actions_from_dotfiles_diff(struct actions *actions,
                                          bool old_link, bool link,
                                          char *module_name) {
  if (old_link) {
    if (!link) {
      get_actions(actions, module_name, DOTFILE, false);
    }
  } else {
    if (link) {
      get_actions(actions, module_name, DOTFILE, true);
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
                                     module.packages, false,
                                     module.name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_packages_diff(actions, old_module.aur_packages,
                                     module.aur_packages, true,
                                     module.name) != EXIT_SUCCESS) {
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
      char *hook = module.pre_root_hooks.items[i];
      if (hook == nullptr) {
        EXIT_FAILURE;
      }
      get_actions(actions, hook, PRE_ROOT_HOOK, is_positive);
    }
    for (size_t i = 0; i < module.pre_user_hooks.count; ++i) {
      char *hook = module.pre_user_hooks.items[i];
      if (hook == nullptr) {
        EXIT_FAILURE;
      }
      get_actions(actions, hook, PRE_USER_HOOK, is_positive);
    }
  }
  if (module.packages.count > 0) {
    get_packages_actions(actions, module.packages, PACKAGE, is_positive,
                         module.name);
  }
  if (module.aur_packages.count > 0) {
    get_packages_actions(actions, module.aur_packages, AUR_PACKAGE, is_positive,
                         module.name);
  }
  for (size_t i = 0; i < module.user_services.count; ++i) {
    char *service = module.user_services.items[i];
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
      char *hook = module.post_root_hooks.items[i];
      if (hook == nullptr) {
        EXIT_FAILURE;
      }
      get_actions(actions, hook, POST_ROOT_HOOK, is_positive);
    }
    for (size_t i = 0; i < module.post_user_hooks.count; ++i) {
      char *hook = module.post_user_hooks.items[i];
      if (hook == nullptr) {
        EXIT_FAILURE;
      }
      get_actions(actions, hook, POST_USER_HOOK, is_positive);
    }
  }
  return EXIT_SUCCESS;
}

int do_action(struct action action, char *aur_helper) {
  switch (action.type) {
  case ROOT_SERVICE:
    if (action.is_positive) {
      LOG(LOG_INFO, "enabling root service: %s", action.payload.name);
      if (execute_service_command(true, true, action.payload.name) !=
          EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to enable root service: %s",
            action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully enabled root service: %s",
          action.payload.name);
    } else {
      LOG(LOG_INFO, "disabling root service: %s", action.payload.name);
      if (execute_service_command(true, false, action.payload.name) !=
          EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to disable root service: %s",
            action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully disabled root service: %s",
          action.payload.name);
    }
    break;
  case USER_SERVICE:
    if (action.is_positive) {
      LOG(LOG_INFO, "enabling user service: %s", action.payload.name);
      if (execute_service_command(false, true, action.payload.name) !=
          EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to enable user service: %s",
            action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully enabled user service: %s",
          action.payload.name);
    } else {
      LOG(LOG_INFO, "disabling user service: %s", action.payload.name);
      if (execute_service_command(false, false, action.payload.name) !=
          EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to disable user service: %s",
            action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully disabled user service: %s",
          action.payload.name);
    }
    break;
  case PRE_ROOT_HOOK:
  case POST_ROOT_HOOK:
    if (action.is_positive) {
      LOG(LOG_INFO, "running hook: %s", action.payload.name);
      if (execute_hook_command(true, action.payload.name) != EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to run hook: %s", action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully ran hook: %s", action.payload.name);
    } else {
      // can't undo a hook...
    }
    break;
  case PRE_USER_HOOK:
  case POST_USER_HOOK:
    if (action.is_positive) {
      LOG(LOG_INFO, "running hook: %s", action.payload.name);
      if (execute_hook_command(false, action.payload.name) != EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to run hook: %s", action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully ran hook: %s", action.payload.name);
    } else {
      // can't undo a hook...
    }
    break;
  case PACKAGE:
    if (action.is_positive) {
      LOG(LOG_INFO, "installing packages for module: %s", action.payload.name);
      if (execute_package_install_command(action.payload.packages) !=
          EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to install packages for module: %s",
            action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully installed packages for module: %s",
          action.payload.name);
    } else {
    package_remove:
      LOG(LOG_INFO, "removing packages for module: %s", action.payload.name);
      if (execute_package_remove_command(action.payload.packages) !=
          EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to remove packages for module: %s",
            action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully removed packages for module: %s",
          action.payload.name);
    }
    break;
  case AUR_PACKAGE:
    if (action.is_positive) {
      LOG(LOG_INFO, "installing aur packages for module: %s",
          action.payload.name);
      if (execute_aur_package_install_command(action.payload.packages,
                                              aur_helper) != EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to install aur packages for module: %s",
            action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully installed aur packages for module: %s",
          action.payload.name);
    } else {
      goto package_remove;
    }
    break;
  case DOTFILE:
    if (action.is_positive) {
      LOG(LOG_INFO, "linking dotfiles for module: %s", action.payload.name);
      if (execute_dotfile_command(true, action.payload.name) != EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to link dotfiles for module: %s",
            action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully linked dotfiles for module: %s",
          action.payload.name);
    } else {
      LOG(LOG_INFO, "unlinking dotfiles for module: %s", action.payload.name);
      if (execute_dotfile_command(false, action.payload.name) != EXIT_SUCCESS) {
        LOG(LOG_ERROR, "failed to link dotfiles for module: %s",
            action.payload.name);
        return EXIT_FAILURE;
      }
      LOG(LOG_INFO, "succesfully linked dotfiles for module: %s",
          action.payload.name);
    }
    break;
  }
  return EXIT_SUCCESS;
}

// TODO: finish the undo action loop?
int undo_action(struct action action, [[maybe_unused]] char *aur_helper) {
  switch (action.type) {
  case ROOT_SERVICE:
    break;
  case USER_SERVICE:
    break;
  case PRE_ROOT_HOOK:
  case POST_ROOT_HOOK:
  case PRE_USER_HOOK:
  case POST_USER_HOOK:
    // can't undo hooks...
    break;
  case PACKAGE:
    break;
  case AUR_PACKAGE:
    break;
  case DOTFILE:
    break;
  }
  return EXIT_SUCCESS;
}
