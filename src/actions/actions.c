#include "damgr/actions.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <string.h>

static void compute_darray_diff(struct darray *negative,
                                struct darray *positive,
                                struct darray *old_array,
                                struct darray *array) {
  qsort(old_array->items, old_array->count, sizeof(old_array->items[0]),
        qcharcmp);
  qsort(array->items, array->count, sizeof(array->items[0]), qcharcmp);
  size_t i = 0;
  size_t j = 0;
  while (i < old_array->count && j < array->count) {
    int ret = strcmp(old_array->items[i], array->items[j]);
    if (ret < 0) {
      if (negative != nullptr) {
        darray_append(negative, old_array->items[i]);
      }
      ++i;
    } else if (ret > 0) {
      darray_append(positive, array->items[j]);
      ++j;
    } else {
      ++i;
      ++j;
    }
  }
  while (i < old_array->count) {
    if (negative != nullptr) {
      darray_append(negative, old_array->items[i]);
    }
    ++i;
  }
  while (j < array->count) {
    darray_append(positive, array->items[j]);
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

static void get_action(struct actions *actions, enum action_type type,
                       bool is_positive, struct payload payload) {
  struct action action = {.payload = payload,
                          .status = PENDING,
                          .type = type,
                          .is_positive = is_positive};
  actions_append(actions, action);
}

static int get_actions_from_services_diff(struct actions *actions,
                                          struct darray *old_services,
                                          struct darray *services, bool root) {
  struct darray to_disable = {};
  struct darray to_enable = {};
  compute_darray_diff(&to_disable, &to_enable, old_services, services);
  for (size_t i = 0; i < to_disable.count; ++i) {
    char *service = to_disable.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    if (root) {
      get_action(actions, ROOT_SERVICE, false, payload);
    } else {
      get_action(actions, USER_SERVICE, false, payload);
    }
  }
  for (size_t i = 0; i < to_enable.count; ++i) {
    char *service = to_enable.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    if (root) {
      get_action(actions, ROOT_SERVICE, true, payload);
    } else {
      get_action(actions, USER_SERVICE, true, payload);
    }
  }
  return EXIT_SUCCESS;
}

static int get_actions_from_hooks_diff(struct actions *actions,
                                       struct darray *old_hooks,
                                       struct darray *hooks, bool root,
                                       bool pre) {
  // hooks can't be undone so no to_undo
  struct darray to_do = {};
  compute_darray_diff(nullptr, &to_do, old_hooks, hooks);
  for (size_t i = 0; i < to_do.count; ++i) {
    char *hook = to_do.items[i];
    if (hook == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {hook, .packages = {}};
    if (root) {
      if (pre) {
        get_action(actions, PRE_ROOT_HOOK, true, payload);
      } else {
        get_action(actions, POST_ROOT_HOOK, true, payload);
      }
    } else {
      if (pre) {
        get_action(actions, PRE_USER_HOOK, true, payload);
      } else {
        get_action(actions, POST_USER_HOOK, true, payload);
      }
    }
  }

  return EXIT_SUCCESS;
}

static int get_actions_from_packages_diff(struct actions *actions,
                                          struct darray *old_packages,
                                          struct darray *packages, bool aur,
                                          char *name) {
  struct darray to_remove = {};
  struct darray to_install = {};
  compute_darray_diff(&to_remove, &to_install, old_packages, packages);
  if (aur && to_remove.count > 0) {
    struct payload payload = {.name = name, .packages = to_remove};
    get_action(actions, AUR_PACKAGE, false, payload);
  } else if (!aur && to_remove.count > 0) {
    struct payload payload = {.name = name, .packages = to_remove};
    get_action(actions, PACKAGE, false, payload);
  }
  if (aur && to_install.count > 0) {
    struct payload payload = {.name = name, .packages = to_install};
    get_action(actions, AUR_PACKAGE, true, payload);
  } else if (!aur && to_install.count > 0) {
    struct payload payload = {.name = name, .packages = to_install};
    get_action(actions, PACKAGE, true, payload);
  }
  return EXIT_SUCCESS;
}

static void get_actions_from_dotfiles_diff(struct module *old_module,
                                           struct module *module) {
  if (old_module->to_link) {
    if (!module->to_link) {
      struct payload payload = {module->name, .packages = {}};
      get_action(&module->module_actions, DOTFILE, false, payload);
    }
  } else {
    if (module->to_link) {
      struct payload payload = {module->name, .packages = {}};
      get_action(&module->module_actions, DOTFILE, true, payload);
    }
  }
}

static int get_actions_from_modules_diff(struct module *old_module,
                                         struct module *module) {
  if (get_actions_from_hooks_diff(
          &module->module_actions, &old_module->pre_root_hooks,
          &module->pre_root_hooks, true, true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_hooks_diff(
          &module->module_actions, &old_module->pre_user_hooks,
          &module->pre_user_hooks, false, true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_packages_diff(&module->module_actions,
                                     &old_module->packages, &module->packages,
                                     false, module->name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_packages_diff(
          &module->module_actions, &old_module->aur_packages,
          &module->aur_packages, true, module->name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_services_diff(
          &module->module_actions, &old_module->user_services,
          &module->user_services, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  // no failure for dotfiles
  get_actions_from_dotfiles_diff(old_module, module);

  if (get_actions_from_hooks_diff(
          &module->module_actions, &old_module->post_root_hooks,
          &module->post_root_hooks, true, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (get_actions_from_hooks_diff(
          &module->module_actions, &old_module->post_user_hooks,
          &module->post_user_hooks, false, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static int get_actions_from_module(struct module *module, bool is_positive) {
  // can't undo hooks
  if (is_positive) {
    for (size_t i = 0; i < module->pre_root_hooks.count; ++i) {
      char *hook = module->pre_root_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      get_action(&module->module_actions, PRE_ROOT_HOOK, is_positive, payload);
    }
    for (size_t i = 0; i < module->pre_user_hooks.count; ++i) {
      char *hook = module->pre_user_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      get_action(&module->module_actions, PRE_USER_HOOK, is_positive, payload);
    }
  }
  if (module->packages.count > 0) {
    struct payload payload = {.name = module->name,
                              .packages = module->packages};
    get_action(&module->module_actions, PACKAGE, is_positive, payload);
  }
  if (module->aur_packages.count > 0) {
    struct payload payload = {.name = module->name,
                              .packages = module->aur_packages};
    get_action(&module->module_actions, AUR_PACKAGE, is_positive, payload);
  }
  for (size_t i = 0; i < module->user_services.count; ++i) {
    char *service = module->user_services.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    get_action(&module->module_actions, USER_SERVICE, is_positive, payload);
  }
  if (module->to_link) {
    struct payload payload = {.name = module->name, .packages = {}};
    get_action(&module->module_actions, DOTFILE, is_positive, payload);
  }
  if (is_positive) {
    for (size_t i = 0; i < module->post_root_hooks.count; ++i) {
      char *hook = module->post_root_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      get_action(&module->module_actions, POST_ROOT_HOOK, is_positive, payload);
    }
    for (size_t i = 0; i < module->post_user_hooks.count; ++i) {
      char *hook = module->post_user_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      get_action(&module->module_actions, POST_ROOT_HOOK, is_positive, payload);
    }
  }
  return EXIT_SUCCESS;
}

static int get_actions_from_hosts_diff(struct host *old_host,
                                       struct host *host) {
  // host->host_actions will also hold the services to disable of the old_host
  if (get_actions_from_services_diff(
          &host->host_actions, &old_host->root_services, &host->root_services,
          true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < host->modules.count; ++i) {
    for (size_t j = 0; j < old_host->modules.count; ++j) {
      // first check if the name lengths are equal, if so perform needle in
      // haystack search, else skip
      if (strlen(old_host->modules.items[j].name) ==
              strlen(host->modules.items[i].name) &&
          string_contains(old_host->modules.items[j].name,
                          host->modules.items[i].name)) {
        old_host->modules.items[j].boolean.is_orphan = false; // to remove later
        if (get_actions_from_modules_diff(&old_host->modules.items[j],
                                          &host->modules.items[i]) !=
            EXIT_SUCCESS) {
          return EXIT_FAILURE;
        } else {
          host->modules.items[i].boolean.is_done = true;
          host->all_modules_actions_count +=
              host->modules.items[i].module_actions.count;
        }
      }
    }
    if (!host->modules.items[i].boolean.is_done) {
      if (get_actions_from_module(&host->modules.items[i], true) !=
          EXIT_SUCCESS) {
        return EXIT_FAILURE;
      } else {
        host->all_modules_actions_count +=
            host->modules.items[i].module_actions.count;
      }
    }
  }
  for (size_t i = 0; i < old_host->modules.count; ++i) {
    if (old_host->modules.items[i].boolean.is_orphan) {
      if (get_actions_from_module(&old_host->modules.items[i], false) !=
          EXIT_SUCCESS) {
        return EXIT_FAILURE;
      } else {
        old_host->all_modules_actions_count +=
            old_host->modules.items[i].module_actions.count;
      }
    }
  }
  return EXIT_SUCCESS;
}

static int get_actions_from_host(struct host *host) {
  for (size_t i = 0; i < host->root_services.count; i++) {
    char *service = host->root_services.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    get_action(&host->host_actions, ROOT_SERVICE, true, payload);
  }
  for (size_t i = 0; i < host->modules.count; i++) {
    if (get_actions_from_module(&host->modules.items[i], true) !=
        EXIT_SUCCESS) {
      host->all_modules_actions_count +=
          host->modules.items[i].module_actions.count;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int get_actions(struct config *old_config, struct config *config) {
  if (old_config->active_host.name != nullptr) {
    int ret = strcmp(old_config->active_host.name, config->active_host.name);
    if (ret < 0 || ret > 0) { // different host
      if (get_actions_from_host(&config->active_host) != EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to get actions for the new host: %s",
                  config->active_host.name);
        return EXIT_FAILURE;
      }
    } else { // same host
      if (get_actions_from_hosts_diff(&old_config->active_host,
                                      &config->active_host) != EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to get actions comparing the hosts: %s",
                  config->active_host.name);
        return EXIT_FAILURE;
      }
    }
  } else { // no state host
    if (get_actions_from_host(&config->active_host) != EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to get actions for the new host: %s",
                config->active_host.name);
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

static int do_action(char *user, struct action *action, char *aur_helper) {
  switch (action->type) {
  case ROOT_SERVICE:
    if (action->is_positive) {
      damgr_log(INFO, "enabling root service: %s", action->payload.name);
      if (execute_service_command(true, true, action->payload.name) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to enable root service: %s",
                  action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully enabled root service: %s",
                action->payload.name);
    } else {
      damgr_log(INFO, "disabling root service: %s", action->payload.name);
      if (execute_service_command(true, false, action->payload.name) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to disable root service: %s",
                  action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully disabled root service: %s",
                action->payload.name);
    }
    break;
  case USER_SERVICE:
    if (action->is_positive) {
      damgr_log(INFO, "enabling user service: %s", action->payload.name);
      if (execute_service_command(false, true, action->payload.name) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to enable user service: %s",
                  action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully enabled user service: %s",
                action->payload.name);
    } else {
      damgr_log(INFO, "disabling user service: %s", action->payload.name);
      if (execute_service_command(false, false, action->payload.name) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to disable user service: %s",
                  action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully disabled user service: %s",
                action->payload.name);
    }
    break;
  case PRE_ROOT_HOOK:
  case POST_ROOT_HOOK:
    if (action->is_positive) {
      damgr_log(INFO, "running hook: %s", action->payload.name);
      if (execute_hook_command(user, true, action->payload.name) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to run hook: %s", action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully ran hook: %s", action->payload.name);
    } else {
      // can't undo a hook...
    }
    break;
  case PRE_USER_HOOK:
  case POST_USER_HOOK:
    if (action->is_positive) {
      damgr_log(INFO, "running hook: %s", action->payload.name);
      if (execute_hook_command(user, false, action->payload.name) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to run hook: %s", action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully ran hook: %s", action->payload.name);
    } else {
      // can't undo a hook...
    }
    break;
  case PACKAGE:
    if (action->is_positive) {
      damgr_log(INFO, "installing packages for module: %s",
                action->payload.name);
      if (execute_package_install_command(action->payload.packages) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to install packages for module: %s",
                  action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully installed packages for module: %s",
                action->payload.name);
    } else {
    package_remove:
      damgr_log(INFO, "removing packages for module: %s", action->payload.name);
      if (execute_package_remove_command(action->payload.packages) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to remove packages for module: %s",
                  action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully removed packages for module: %s",
                action->payload.name);
    }
    break;
  case AUR_PACKAGE:
    if (action->is_positive) {
      damgr_log(INFO, "installing aur packages for module: %s",
                action->payload.name);
      if (execute_aur_package_install_command(action->payload.packages,
                                              aur_helper) != EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to install aur packages for module: %s",
                  action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully installed aur packages for module: %s",
                action->payload.name);
    } else {
      goto package_remove;
    }
    break;
  case DOTFILE:
    if (action->is_positive) {
      damgr_log(INFO, "linking dotfiles for module: %s", action->payload.name);
      if (execute_dotfile_command(user, true, action->payload.name) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to link dotfiles for module: %s",
                  action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully linked dotfiles for module: %s",
                action->payload.name);
    } else {
      damgr_log(INFO, "unlinking dotfiles for module: %s",
                action->payload.name);
      if (execute_dotfile_command(user, false, action->payload.name) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to link dotfiles for module: %s",
                  action->payload.name);
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "succesfully linked dotfiles for module: %s",
                action->payload.name);
    }
    break;
  }
  return EXIT_SUCCESS;
}

static void do_module_actions(char *user, struct module *module,
                              char *aur_helper) {
  size_t actions_succeeded_count = 0;
  for (size_t j = 0; j < module->module_actions.count; ++j) {
    struct action *action = &module->module_actions.items[j];
    if (do_action(user, action, aur_helper) != EXIT_SUCCESS) {
      action->status = FAILED;
      break; // do we have to break when an action fails?
    } else {
      action->status = SUCCEEDED;
      actions_succeeded_count += 1;
    }
  }
  if (actions_succeeded_count != module->module_actions.count) {
    module->boolean.is_done = false; // failed modules will not be written
  } else {
    // all module_actions succeeded
    module->boolean.is_done = true; // module can be written to state
  }
}

int do_actions(char *user, struct config *old_config, struct config *config) {
  // config->active_host.host_actions will also hold the services to disable of
  // the old_host
  for (size_t i = 0; i < config->active_host.host_actions.count; ++i) {
    struct action *action = &config->active_host.host_actions.items[i];
    if (do_action(user, action, config->aur_helper) != EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to do host actions for the host: %s",
                config->active_host.name);
      action->status = FAILED;
      return EXIT_FAILURE;
    } else {
      action->status = SUCCEEDED;
    }
  }

  for (size_t i = 0; i < config->active_host.modules.count; ++i) {
    do_module_actions(user, &config->active_host.modules.items[i],
                      config->aur_helper);
  }
  for (size_t i = 0; i < old_config->active_host.modules.count; ++i) {
    do_module_actions(user, &old_config->active_host.modules.items[i],
                      config->aur_helper);
  }
  return EXIT_SUCCESS;
}
