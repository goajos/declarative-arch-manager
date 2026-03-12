#include "damgr/actions.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include <string.h>

static void damgr_queue_append(Damgr_Action_Queue *queue, Damgr_Action action) {
  if (queue->count >= queue->capacity) {
    if (queue->capacity == 0) {
      queue->capacity = 16;
    } else {
      queue->capacity *= 2;
    }
    queue->items =
        realloc(queue->items, queue->capacity * sizeof(*queue->items));
  }
  queue->items[queue->count++] = action;
}

static void damgr_actions_append(Damgr_Actions *actions,
                                 Damgr_Action_Queue queue) {
  if (actions->count >= actions->capacity) {
    if (actions->capacity == 0) {
      actions->capacity = 16;
    } else {
      actions->capacity *= 2;
    }
    actions->queues =
        realloc(actions->queues, actions->capacity * sizeof(*actions->queues));
  }
  actions->queues[actions->count++] = queue;
}

static void damgr_get_action(Damgr_Action_Queue *queue, Damgr_Action_Type type,
                             bool is_action_positive,
                             Damgr_Action_Payload payload) {
  struct action action = {.payload = payload,
                          .status = PENDING,
                          .type = type,
                          .is_action_positive = is_action_positive};
  damgr_queue_append(queue, action);
}

static int get_actions_from_module(Damgr_Action_Queue *queue,
                                   Damgr_Module *module,
                                   bool is_action_positive) {
  // can't undo hooks
  if (is_action_positive) {
    for (size_t i = 0; i < module->pre_root_hooks.count; ++i) {
      char *hook = module->pre_root_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      damgr_get_action(queue, PRE_ROOT_HOOK, is_action_positive, payload);
    }
    for (size_t i = 0; i < module->pre_user_hooks.count; ++i) {
      char *hook = module->pre_user_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      damgr_get_action(queue, PRE_USER_HOOK, is_action_positive, payload);
    }
  }
  if (module->packages.count > 0) {
    struct payload payload = {.name = module->name,
                              .packages = module->packages};
    damgr_get_action(queue, PACKAGE, is_action_positive, payload);
  }
  if (module->aur_packages.count > 0) {
    struct payload payload = {.name = module->name,
                              .packages = module->aur_packages};
    damgr_get_action(queue, AUR_PACKAGE, is_action_positive, payload);
  }
  for (size_t i = 0; i < module->user_services.count; ++i) {
    char *service = module->user_services.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    damgr_get_action(queue, USER_SERVICE, is_action_positive, payload);
  }
  if (module->to_link) {
    struct payload payload = {.name = module->name, .packages = {}};
    damgr_get_action(queue, DOTFILE, is_action_positive, payload);
  }
  if (is_action_positive) {
    for (size_t i = 0; i < module->post_root_hooks.count; ++i) {
      char *hook = module->post_root_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      damgr_get_action(queue, POST_ROOT_HOOK, is_action_positive, payload);
    }
    for (size_t i = 0; i < module->post_user_hooks.count; ++i) {
      char *hook = module->post_user_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      damgr_get_action(queue, POST_ROOT_HOOK, is_action_positive, payload);
    }
  }
  damgr_log(INFO, "successfully got %zu actions for module: %s", queue->count,
            module->name);
  return EXIT_SUCCESS;
}

static int damgr_get_actions_from_host(Damgr_Actions *actions,
                                       Damgr_Host *host) {
  for (size_t i = 0; i < host->root_services.count; i++) {
    char *service = host->root_services.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    // host queue is always index 0
    Damgr_Action_Queue host_queue = {};
    damgr_actions_append(actions, host_queue);
    damgr_get_action(&actions->queues[0], ROOT_SERVICE, true, payload);
  }
  damgr_log(INFO, "successfully got %zu actions for host: %s",
            actions->queues[0].count, host->name);
  for (size_t i = 0; i < host->modules.count; i++) {
    // module queues are index i+1
    Damgr_Action_Queue module_queue = {};
    damgr_actions_append(actions, module_queue);
    if (get_actions_from_module(&actions->queues[i + 1],
                                &host->modules.items[i],
                                true) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int damgr_get_actions(Damgr_Actions *actions, Damgr_Config *old_config,
                      Damgr_Config *config) {
  if (old_config->active_host.name != nullptr) {
    int ret = strcmp(old_config->active_host.name, config->active_host.name);
    if (ret < 0 || ret > 0) { // different host
      if (damgr_get_actions_from_host(actions, &config->active_host) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to get actions for the new host: %s",
                  config->active_host.name);
        return EXIT_FAILURE;
      }
    } else { // same host
      // if (damgr_get_actions_from_hosts_diff(
      //         &old_config->active_host, &config->active_host) !=
      //         EXIT_SUCCESS) {
      //   damgr_log(ERROR, "failed to get actions comparing the hosts: %s",
      //             config->active_host.name);
      //   return EXIT_FAILURE;
      // }
    }
  } else { // no state host
    if (damgr_get_actions_from_host(actions, &config->active_host) !=
        EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to get actions for the new host: %s",
                config->active_host.name);
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
