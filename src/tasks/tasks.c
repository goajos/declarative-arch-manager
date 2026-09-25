#include "damgr/tasks.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <string.h>

const char *damgr_task_type_keys[] = {
    [ROOT_SERVICE] = "root service",
    [PRE_ROOT_HOOK] = "pre root hook",
    [PRE_USER_HOOK] = "pre user hook",
    [PACKAGE] = "package",
    [AUR_PACKAGE] = "aur package",
    [USER_SERVICE] = "user service",
    [DOTFILE] = "dotfile",
    [POST_ROOT_HOOK] = "post root hook",
    [POST_USER_HOOK] = "post user hook",
};

static void damgr_compute_darray_diff(Damgr_Darray *negative,
                                      Damgr_Darray *positive,
                                      Damgr_Darray old_array,
                                      Damgr_Darray array) {
  qsort(old_array.items, old_array.count, sizeof(old_array.items[0]),
        damgr_qcharcmp);
  qsort(array.items, array.count, sizeof(array.items[0]), damgr_qcharcmp);
  size_t i = 0;
  size_t j = 0;
  while (i < old_array.count && j < array.count) {
    int ret = strcmp(old_array.items[i], array.items[j]);
    if (ret < 0) {
      if (negative != nullptr) {
        damgr_darray_append(negative, old_array.items[i]);
      }
      ++i;
    } else if (ret > 0) {
      damgr_darray_append(positive, array.items[j]);
      ++j;
    } else {
      ++i;
      ++j;
    }
  }
  while (i < old_array.count) {
    if (negative != nullptr) {
      damgr_darray_append(negative, old_array.items[i]);
    }
    ++i;
  }
  while (j < array.count) {
    damgr_darray_append(positive, array.items[j]);
    ++j;
  }
}

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
                           bool is_new_state, Damgr_Task_Payload payload,
                           char *name) {
  struct task task = {.name = name,
                      .payload = payload,
                      .status = PENDING,
                      .type = type,
                      .is_new_state = is_new_state};
  damgr_queue_append(queue, task);
}

static int get_tasks_from_module(Damgr_Task_Queue *queue, Damgr_Module module,
                                 bool is_new_state) {
  // can't undo hooks
  if (is_new_state) {
    for (size_t i = 0; i < module.pre_root_hooks.count; ++i) {
      char *hook = module.pre_root_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.payload_name = hook};
      damgr_get_task(queue, PRE_ROOT_HOOK, is_new_state, payload, module.name);
    }
    for (size_t i = 0; i < module.pre_user_hooks.count; ++i) {
      char *hook = module.pre_user_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.payload_name = hook};
      damgr_get_task(queue, PRE_USER_HOOK, is_new_state, payload, module.name);
    }
  }
  if (module.packages.count > 0) {
    struct payload payload = {.packages = module.packages};
    damgr_get_task(queue, PACKAGE, is_new_state, payload, module.name);
  }
  if (module.aur_packages.count > 0) {
    struct payload payload = {.packages = module.aur_packages};
    damgr_get_task(queue, AUR_PACKAGE, is_new_state, payload, module.name);
  }
  for (size_t i = 0; i < module.user_services.count; ++i) {
    char *service = module.user_services.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.payload_name = service};
    damgr_get_task(queue, USER_SERVICE, is_new_state, payload, module.name);
  }
  if (module.to_link) {
    struct payload payload = {.payload_name = module.name};
    damgr_get_task(queue, DOTFILE, is_new_state, payload, module.name);
  }
  if (is_new_state) {
    for (size_t i = 0; i < module.post_root_hooks.count; ++i) {
      char *hook = module.post_root_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.payload_name = hook};
      damgr_get_task(queue, POST_ROOT_HOOK, is_new_state, payload, module.name);
    }
    for (size_t i = 0; i < module.post_user_hooks.count; ++i) {
      char *hook = module.post_user_hooks.items[i];
      if (hook == nullptr) {
        return EXIT_FAILURE;
      }
      struct payload payload = {.payload_name = hook};
      damgr_get_task(queue, POST_ROOT_HOOK, is_new_state, payload, module.name);
    }
  }
  return EXIT_SUCCESS;
}

static int damgr_get_tasks_from_services_diff(Damgr_Task_Queue *queue,
                                              Damgr_Darray old_services,
                                              Damgr_Darray services,
                                              Damgr_Task_Type type,
                                              char *name) {
  struct darray to_disable = {};
  struct darray to_enable = {};
  damgr_compute_darray_diff(&to_disable, &to_enable, old_services, services);
  for (size_t i = 0; i < to_disable.count; ++i) {
    char *service = to_disable.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.payload_name = service};
    damgr_get_task(queue, type, false, payload, name);
  }
  for (size_t i = 0; i < to_enable.count; ++i) {
    char *service = to_enable.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.payload_name = service};
    damgr_get_task(queue, type, true, payload, name);
  }
  return EXIT_SUCCESS;
}

static int damgr_get_tasks_from_hooks_diff(Damgr_Task_Queue *queue,
                                           Damgr_Darray old_hooks,
                                           Damgr_Darray hooks,
                                           Damgr_Task_Type type, char *name) {
  // can't undo hooks
  struct darray to_run = {};
  damgr_compute_darray_diff(nullptr, &to_run, old_hooks, hooks);
  for (size_t i = 0; i < to_run.count; ++i) {
    char *hook = to_run.items[i];
    if (hook == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.payload_name = hook};
    damgr_get_task(queue, type, true, payload, name);
  }
  return EXIT_SUCCESS;
}

static int damgr_get_tasks_from_packages_diff(Damgr_Task_Queue *queue,
                                              Damgr_Darray old_packages,
                                              Damgr_Darray packages,
                                              char *name) {
  struct darray to_install = {};
  struct darray to_remove = {};
  damgr_compute_darray_diff(&to_install, &to_remove, old_packages, packages);
  if (to_install.count > 0) {
    struct payload payload = {.packages = to_install};
    damgr_get_task(queue, PACKAGE, true, payload, name);
  }
  if (to_remove.count > 0) {
    struct payload payload = {.packages = to_remove};
    damgr_get_task(queue, PACKAGE, false, payload, name);
  }

  return EXIT_SUCCESS;
}

static int damgr_get_tasks_from_module_diff(Damgr_Task_Queue *queue,
                                            Damgr_Module old_module,
                                            Damgr_Module module) {
  if (damgr_get_tasks_from_services_diff(queue, old_module.user_services,
                                         module.user_services, USER_SERVICE,
                                         module.name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (damgr_get_tasks_from_packages_diff(queue, old_module.packages,
                                         module.packages,
                                         module.name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (damgr_get_tasks_from_packages_diff(queue, old_module.aur_packages,
                                         module.aur_packages,
                                         module.name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (damgr_get_tasks_from_hooks_diff(queue, old_module.pre_root_hooks,
                                      module.pre_root_hooks, PRE_ROOT_HOOK,
                                      module.name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (damgr_get_tasks_from_hooks_diff(queue, old_module.pre_user_hooks,
                                      module.pre_user_hooks, PRE_USER_HOOK,
                                      module.name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (damgr_get_tasks_from_hooks_diff(queue, old_module.post_root_hooks,
                                      module.post_root_hooks, POST_ROOT_HOOK,
                                      module.name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (damgr_get_tasks_from_hooks_diff(queue, old_module.post_user_hooks,
                                      module.post_user_hooks, POST_USER_HOOK,
                                      module.name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static int damgr_get_tasks_from_hosts_diff(Damgr_Tasks *tasks,
                                           Damgr_Host old_host,
                                           Damgr_Host host) {
  Damgr_Task_Queue host_queue = {};
  damgr_tasks_append(tasks, host_queue);
  if (damgr_get_tasks_from_services_diff(&host_queue, old_host.root_services,
                                         host.root_services, ROOT_SERVICE,
                                         host.name) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  damgr_log(INFO, "successfully got %zu tasks for host: %s", host_queue.count,
            host.name);
  damgr_tasks_append(tasks, host_queue);
  for (size_t i = 0; i < host.modules.count; ++i) {
    Damgr_Task_Queue module_queue = {};
    Damgr_Module module = host.modules.items[i];
    for (size_t j = 0; j < old_host.modules.count; ++j) {
      Damgr_Module old_module = old_host.modules.items[j];
      // first check if the name lengths are equal, if so perform needle in
      // haystack search, else skip
      if (strlen(old_module.name) == strlen(module.name) &&
          damgr_string_contains(old_module.name, module.name)) {
        old_module.is_orphan = false; // to remove later
        if (damgr_get_tasks_from_module_diff(&module_queue, old_module,
                                             module) != EXIT_SUCCESS) {
          return EXIT_FAILURE;
        }
        module.is_compared = true; // to skip later
        damgr_log(INFO,
                  "successfully got %zu tasks after comparison for module: %s",
                  module_queue.count, module.name);
        damgr_tasks_append(tasks, module_queue);
        break;
      }
    }
    if (!module.is_compared) {
      if (get_tasks_from_module(&module_queue, module, true) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "successfully got %zu tasks for module: %s",
                module_queue.count, module.name);
      damgr_tasks_append(tasks, module_queue);
    }
  }
  // old modules need cleanup (is_new_state=false)
  for (size_t i = 0; i < old_host.modules.count; ++i) {
    if (old_host.modules.items[i].is_orphan) {
      Damgr_Task_Queue module_queue = {};
      if (get_tasks_from_module(&module_queue, old_host.modules.items[i],
                                false) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
      damgr_log(INFO, "successfully got %zu tasks for module: %s",
                module_queue.count, old_host.modules.items[i].name);
      damgr_tasks_append(tasks, module_queue);
    }
  }
  return EXIT_SUCCESS;
}

static int damgr_get_tasks_from_host(Damgr_Tasks *tasks, Damgr_Host host) {
  Damgr_Task_Queue host_queue = {};
  for (size_t i = 0; i < host.root_services.count; i++) {
    char *service = host.root_services.items[i];
    if (service == nullptr) {
      return EXIT_FAILURE;
    }
    struct payload payload = {.payload_name = service};
    damgr_get_task(&host_queue, ROOT_SERVICE, true, payload, host.name);
  }
  if (host_queue.count > 0) {
    damgr_log(INFO, "successfully got %zu tasks for host: %s", host_queue.count,
              host.name);
    damgr_tasks_append(tasks, host_queue);
  }
  for (size_t i = 0; i < host.modules.count; i++) {
    Damgr_Task_Queue module_queue = {};
    if (get_tasks_from_module(&module_queue, host.modules.items[i], true) !=
        EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
    damgr_log(INFO, "successfully got %zu tasks for module: %s",
              module_queue.count, host.modules.items[i].name);
    damgr_tasks_append(tasks, module_queue);
  }
  return EXIT_SUCCESS;
}

int damgr_get_tasks(Damgr_Tasks *tasks, Damgr_Config old_config,
                    Damgr_Config config) {
  if (old_config.active_host.name != nullptr) {
    int ret = strcmp(old_config.active_host.name, config.active_host.name);
    if (ret < 0 || ret > 0) { // different host
      if (damgr_get_tasks_from_host(tasks, config.active_host) !=
          EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to get tasks for the new host: %s",
                  config.active_host.name);
        return EXIT_FAILURE;
      }
    } else { // same host
      if (damgr_get_tasks_from_hosts_diff(tasks, old_config.active_host,
                                          config.active_host) != EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to get tasks comparing the hosts: %s",
                  config.active_host.name);
        return EXIT_FAILURE;
      }
    }
  } else { // no state host
    if (damgr_get_tasks_from_host(tasks, config.active_host) != EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to get tasks for the new host: %s",
                config.active_host.name);
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

// TODO: should this return exit status?
static void damgr_do_task(Damgr_Task *task, char *aur_helper, char *user) {
  switch (task->type) {
  case ROOT_SERVICE:
    if (damgr_execute_service_command(true, task->is_new_state,
                                      task->payload.payload_name) !=
        EXIT_SUCCESS) {
      task->status = FAILED;
    } else {
      task->status = SUCCEEDED;
    }
    break;
  case PRE_ROOT_HOOK:
    if (damgr_execute_hook_command(user, true, task->payload.payload_name) !=
        EXIT_SUCCESS) {
      task->status = FAILED;
    } else {
      task->status = SUCCEEDED;
    }
    break;
  case PRE_USER_HOOK:
    if (damgr_execute_hook_command(user, false, task->payload.payload_name) !=
        EXIT_SUCCESS) {
      task->status = FAILED;
    } else {
      task->status = SUCCEEDED;
    }
    break;
  case PACKAGE:
    if (task->is_new_state) {
      if (damgr_execute_package_install_command(task->payload.packages) !=
          EXIT_SUCCESS) {
        task->status = FAILED;
      } else {
        task->status = SUCCEEDED;
      }
    } else {
      if (damgr_execute_package_remove_command(task->payload.packages) !=
          EXIT_SUCCESS) {
        task->status = FAILED;
      } else {
        task->status = SUCCEEDED;
      }
    }
    break;
  case AUR_PACKAGE:
    if (task->is_new_state) {
      if (damgr_execute_aur_package_install_command(
              task->payload.packages, aur_helper) != EXIT_SUCCESS) {
        task->status = FAILED;
      } else {
        task->status = SUCCEEDED;
      }
    } else {
      if (damgr_execute_package_remove_command(task->payload.packages) !=
          EXIT_SUCCESS) {
        task->status = FAILED;
      } else {
        task->status = SUCCEEDED;
      }
    }
    break;
  case USER_SERVICE:
    if (damgr_execute_service_command(false, task->is_new_state,
                                      task->payload.payload_name) !=
        EXIT_SUCCESS) {
      task->status = FAILED;
    } else {
      task->status = SUCCEEDED;
    }
    break;
  case DOTFILE:
    if (damgr_execute_dotfile_command(user, task->is_new_state,
                                      task->payload.payload_name) !=
        EXIT_SUCCESS) {
      task->status = FAILED;
    } else {
      task->status = SUCCEEDED;
    }
    break;
  case POST_ROOT_HOOK:
    if (damgr_execute_hook_command(user, true, task->payload.payload_name) !=
        EXIT_SUCCESS) {
      task->status = FAILED;
    } else {
      task->status = SUCCEEDED;
    }
    break;
  case POST_USER_HOOK:
    if (damgr_execute_hook_command(user, false, task->payload.payload_name) !=
        EXIT_SUCCESS) {
      task->status = FAILED;
    } else {
      task->status = SUCCEEDED;
    }
    break;
  }
}

int damgr_do_tasks(Damgr_Tasks *tasks, char *aur_helper, char *user,
                   Damgr_Task_Queue *succeeded_queue) {
  for (size_t i = 0; i < tasks->count; ++i) {
    size_t queue_task_count =
        tasks->queues[i].count > 0 ? tasks->queues[i].count : 0;
    for (size_t j = 0; j < queue_task_count; ++j) {
      Damgr_Task task = tasks->queues[i].items[j];
      damgr_do_task(&task, aur_helper, user);
      if (task.status == SUCCEEDED) {
        if (task.type != PACKAGE && task.type != AUR_PACKAGE) {
          damgr_log(INFO, "%s task from %s done: %s",
                    damgr_task_type_keys[task.type], task.name,
                    task.payload.payload_name);
        } else {
          damgr_log(INFO, "%s task from %s done",
                    damgr_task_type_keys[task.type], task.name);
        }
        damgr_queue_append(succeeded_queue, task);
      } else if (task.status == FAILED) {
        if (task.type != PACKAGE && task.type != AUR_PACKAGE) {
          damgr_log(ERROR, "%s task failed from %s: %s",
                    damgr_task_type_keys[task.type], task.name,
                    task.payload.payload_name);
        } else {
          damgr_log(ERROR, "%s task failed from %s",
                    damgr_task_type_keys[task.type], task.name);
        }
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}

// TODO: should this return an exit status?
static void damgr_undo_task([[maybe_unused]] Damgr_Task task) {
  // TODO: implement the undo task logic
}

int damgr_undo_tasks(Damgr_Task_Queue *succeeded_queue) {
  for (size_t i = 0; i < succeeded_queue->count; ++i) {
    damgr_undo_task(succeeded_queue->items[i]);
  }

  return EXIT_SUCCESS;
}
