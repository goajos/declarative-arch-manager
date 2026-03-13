#include "damgr/tasks.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <string.h>

static void damgr_queue_append(Damgr_Task_Queue *queue, Damgr_Task task) {
  if (queue->count >= queue->capacity) {
    if (queue->capacity == 0) {
      queue->capacity = 16;
    } else {
      queue->capacity *= 2;
    }
    queue->items =
        realloc(queue->items, queue->capacity * sizeof(*queue->items));
  }
  queue->items[queue->count++] = task;
}

static void damgr_tasks_append(Damgr_Tasks *tasks, Damgr_Task_Queue queue) {
  if (tasks->count >= tasks->capacity) {
    if (tasks->capacity == 0) {
      tasks->capacity = 16;
    } else {
      tasks->capacity *= 2;
    }
    tasks->queues =
        realloc(tasks->queues, tasks->capacity * sizeof(*tasks->queues));
  }
  tasks->queues[tasks->count++] = queue;
}

static void damgr_get_task(Damgr_Task_Queue *queue, Damgr_Task_Type type,
                           bool is_new_state, Damgr_Task_Payload payload) {
  struct task task = {.payload = payload,
                      .status = PENDING,
                      .type = type,
                      .is_new_state = is_new_state};
  damgr_queue_append(queue, task);
}

static int get_tasks_from_module(Damgr_Task_Queue *queue, Damgr_Module *module,
                                 bool is_task_positive) {
  // can't undo hooks
  if (is_task_positive) {
    for (size_t i = 0; i < module->pre_root_hooks.count; ++i) {
      char *hook = module->pre_root_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      damgr_get_task(queue, PRE_ROOT_HOOK, is_task_positive, payload);
    }
    for (size_t i = 0; i < module->pre_user_hooks.count; ++i) {
      char *hook = module->pre_user_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      damgr_get_task(queue, PRE_USER_HOOK, is_task_positive, payload);
    }
  }
  if (module->packages.count > 0) {
    struct payload payload = {.name = module->name,
                              .packages = module->packages};
    damgr_get_task(queue, PACKAGE, is_task_positive, payload);
  }
  if (module->aur_packages.count > 0) {
    struct payload payload = {.name = module->name,
                              .packages = module->aur_packages};
    damgr_get_task(queue, AUR_PACKAGE, is_task_positive, payload);
  }
  for (size_t i = 0; i < module->user_services.count; ++i) {
    char *service = module->user_services.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    damgr_get_task(queue, USER_SERVICE, is_task_positive, payload);
  }
  if (module->to_link) {
    struct payload payload = {.name = module->name, .packages = {}};
    damgr_get_task(queue, DOTFILE, is_task_positive, payload);
  }
  if (is_task_positive) {
    for (size_t i = 0; i < module->post_root_hooks.count; ++i) {
      char *hook = module->post_root_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      damgr_get_task(queue, POST_ROOT_HOOK, is_task_positive, payload);
    }
    for (size_t i = 0; i < module->post_user_hooks.count; ++i) {
      char *hook = module->post_user_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.name = hook, .packages = {}};
      damgr_get_task(queue, POST_ROOT_HOOK, is_task_positive, payload);
    }
  }
  damgr_log(INFO, "successfully got %zu tasks for module: %s", queue->count,
            module->name);
  return EXIT_SUCCESS;
}

static int damgr_get_tasks_from_services_diff(Damgr_Task_Queue *queue,
                                              Damgr_Darray *old_services,
                                              Damgr_Darray *services,
                                              bool root) {
  struct darray to_disable = {};
  struct darray to_enable = {};
  damgr_compute_darray_diff(&to_disable, &to_enable, old_services, services);
  for (size_t i = 0; i < to_disable.count; ++i) {
    char *service = to_disable.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    if (root) {
      damgr_get_task(queue, ROOT_SERVICE, false, payload);
    } else {
      damgr_get_task(queue, USER_SERVICE, false, payload);
    }
  }
  for (size_t i = 0; i < to_enable.count; ++i) {
    char *service = to_enable.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    if (root) {
      damgr_get_task(queue, ROOT_SERVICE, true, payload);
    } else {
      damgr_get_task(queue, USER_SERVICE, true, payload);
    }
  }
  return EXIT_SUCCESS;
}

static int damgr_get_tasks_from_hosts_diff(Damgr_Tasks *tasks,
                                           Damgr_Host *old_host,
                                           Damgr_Host *host) {
  // TODO: should this be wrapped in a single constructor?
  // host queue is always index 0
  Damgr_Task_Queue host_queue = {};
  damgr_tasks_append(tasks, host_queue);
  if (damgr_get_tasks_from_services_diff(
          &tasks->queues[0], &old_host->root_services, &host->root_services,
          true) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < host->modules.count; ++i) {
    // TODO: should this be wrapped in a single constructor?
    // module queues are index i+1
    Damgr_Task_Queue module_queue = {};
    damgr_tasks_append(tasks, module_queue);
    for (size_t j = 0; j < old_host->modules.count; ++j) {
      // first check if the name lengths are equal, if so perform needle in
      // haystack search, else skip
      if (strlen(old_host->modules.items[j].name) ==
              strlen(host->modules.items[i].name) &&
          damgr_string_contains(old_host->modules.items[j].name,
                                host->modules.items[i].name)) {
        old_host->modules.items[j].module_state.is_orphan =
            false; // to remove later
        if (damgr_get_tasks_from_modules_diff(
                &tasks->queues[i + 1], old_host->modules.items[j],
                &host->modules.items[i]) != EXIT_SUCCESS) {
          return EXIT_FAILURE;
        } else {
          host->modules.items[i].module_state.is_done = true;
        }
      }
    }
    if (!host->modules.items[i].module_state.is_done) {
      if (get_tasks_from_module(&tasks->queues[i + 1], &host->modules.items[i],
                                true) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
    }
  }
  // TODO: what index should old modules get here???
  // TODO: should this be wrapped in a single constructor?
  // module queues are index i+1
  Damgr_Task_Queue module_queue = {};
  damgr_tasks_append(tasks, module_queue);
  for (size_t i = 0; i < old_host->modules.count; ++i) {
    if (old_host->modules.items[i].module_state.is_orphan) {
      if (get_tasks_from_module(&old_host->modules.items[i], false) !=
          EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}

static int damgr_get_tasks_from_host(Damgr_Tasks *tasks, Damgr_Host *host) {
  for (size_t i = 0; i < host->root_services.count; i++) {
    char *service = host->root_services.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.name = service, .packages = {}};
    // host queue is always index 0
    Damgr_Task_Queue host_queue = {};
    damgr_tasks_append(tasks, host_queue);
    damgr_get_task(&tasks->queues[0], ROOT_SERVICE, true, payload);
  }
  damgr_log(INFO, "successfully got %zu tasks for host: %s",
            tasks->queues[0].count, host->name);
  for (size_t i = 0; i < host->modules.count; i++) {
    // module queues are index i+1
    Damgr_Task_Queue module_queue = {};
    damgr_tasks_append(tasks, module_queue);
    if (get_tasks_from_module(&tasks->queues[i + 1], &host->modules.items[i],
                              true) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int damgr_get_tasks(Damgr_Tasks *tasks, Damgr_Config *old_config,
                    Damgr_Config *config) {
  if (old_config->active_host.name != nullptr) {
    int ret = strcmp(old_config->active_host.name, config->active_host.name);
    if (ret < 0 || ret > 0) { // different host
      if (damgr_get_tasks_from_host(tasks, &config->active_host) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to get tasks for the new host: %s",
                  config->active_host.name);
        return EXIT_FAILURE;
      }
    } else { // same host
      if (damgr_get_tasks_from_hosts_diff(tasks, &old_config->active_host,
                                          &config->active_host) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to get tasks comparing the hosts: %s",
                  config->active_host.name);
        return EXIT_FAILURE;
      }
    }
  } else { // no state host
    if (damgr_get_tasks_from_host(tasks, &config->active_host) !=
        EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to get tasks for the new host: %s",
                config->active_host.name);
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
