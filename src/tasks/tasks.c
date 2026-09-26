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
                           bool is_new_state, Damgr_Task_Payload payload) {
  struct task task = {.payload = payload,
                      .status = PENDING,
                      .type = type,
                      .is_new_state = is_new_state};
  damgr_queue_append(queue, task);
}

static void get_tasks_from_module(Damgr_Task_Queue *queue, Damgr_Module module,
                                  bool is_new_state) {
  // can't undo hooks
  if (is_new_state) {
    for (size_t i = 0; i < module.pre_root_hooks.count; ++i) {
      char *hook = module.pre_root_hooks.items[i];
      struct payload payload = {.payload_name = hook};
      damgr_get_task(queue, PRE_ROOT_HOOK, is_new_state, payload);
    }
    for (size_t i = 0; i < module.pre_user_hooks.count; ++i) {
      char *hook = module.pre_user_hooks.items[i];
      struct payload payload = {.payload_name = hook};
      damgr_get_task(queue, PRE_USER_HOOK, is_new_state, payload);
    }
  }
  if (module.packages.count > 0) {
    struct payload payload = {.packages = module.packages};
    damgr_get_task(queue, PACKAGE, is_new_state, payload);
  }
  if (module.aur_packages.count > 0) {
    struct payload payload = {.packages = module.aur_packages};
    damgr_get_task(queue, AUR_PACKAGE, is_new_state, payload);
  }
  for (size_t i = 0; i < module.user_services.count; ++i) {
    char *service = module.user_services.items[i];
    struct payload payload = {.payload_name = service};
    damgr_get_task(queue, USER_SERVICE, is_new_state, payload);
  }
  if (module.to_link) {
    struct payload payload = {.payload_name = module.name};
    damgr_get_task(queue, DOTFILE, is_new_state, payload);
  }
  if (is_new_state) {
    for (size_t i = 0; i < module.post_root_hooks.count; ++i) {
      char *hook = module.post_root_hooks.items[i];
      struct payload payload = {.payload_name = hook};
      damgr_get_task(queue, POST_ROOT_HOOK, is_new_state, payload);
    }
    for (size_t i = 0; i < module.post_user_hooks.count; ++i) {
      char *hook = module.post_user_hooks.items[i];
      struct payload payload = {.payload_name = hook};
      damgr_get_task(queue, POST_ROOT_HOOK, is_new_state, payload);
    }
  }
}

static void damgr_get_tasks_from_services_diff(Damgr_Task_Queue *queue,
                                               Damgr_Darray old_services,
                                               Damgr_Darray services,
                                               Damgr_Task_Type type) {
  Damgr_Darray to_disable = {};
  Damgr_Darray to_enable = {};
  damgr_compute_darray_diff(&to_disable, &to_enable, old_services, services);
  for (size_t i = 0; i < to_disable.count; ++i) {
    char *service = to_disable.items[i];
    struct payload payload = {.payload_name = service};
    damgr_get_task(queue, type, false, payload);
  }
  for (size_t i = 0; i < to_enable.count; ++i) {
    char *service = to_enable.items[i];
    struct payload payload = {.payload_name = service};
    damgr_get_task(queue, type, true, payload);
  }
}

static void damgr_get_tasks_from_root_services_diff(
    Damgr_Task_Queue *queue, Damgr_Root_Services old_services,
    Damgr_Root_Services services, Damgr_Task_Type type) {
  Damgr_Darray old_services_darray = {};
  Damgr_Darray services_darray = {};
  for (size_t i = 0; i < old_services.count; ++i) {
    damgr_darray_append(&old_services_darray, old_services.items[i]);
  }
  for (size_t i = 0; i < services.count; ++i) {
    damgr_darray_append(&services_darray, services.items[i]);
  }
  damgr_get_tasks_from_services_diff(queue, old_services_darray,
                                     services_darray, type);
}

static void damgr_get_tasks_from_hooks_diff(Damgr_Task_Queue *queue,
                                            Damgr_Darray old_hooks,
                                            Damgr_Darray hooks,
                                            Damgr_Task_Type type) {
  // can't undo hooks
  Damgr_Darray to_run = {};
  damgr_compute_darray_diff(nullptr, &to_run, old_hooks, hooks);
  for (size_t i = 0; i < to_run.count; ++i) {
    char *hook = to_run.items[i];
    struct payload payload = {.payload_name = hook};
    damgr_get_task(queue, type, true, payload);
  }
}

static void damgr_get_tasks_from_packages_diff(Damgr_Task_Queue *queue,
                                               Damgr_Darray old_packages,
                                               Damgr_Darray packages) {
  Damgr_Darray to_install = {};
  Damgr_Darray to_remove = {};
  damgr_compute_darray_diff(&to_install, &to_remove, old_packages, packages);
  if (to_install.count > 0) {
    struct payload payload = {.packages = to_install};
    damgr_get_task(queue, PACKAGE, true, payload);
  }
  if (to_remove.count > 0) {
    struct payload payload = {.packages = to_remove};
    damgr_get_task(queue, PACKAGE, false, payload);
  }
}

static void damgr_get_tasks_from_module_diff(Damgr_Task_Queue *queue,
                                             Damgr_Module old_module,
                                             Damgr_Module module) {
  damgr_get_tasks_from_services_diff(queue, old_module.user_services,
                                     module.user_services, USER_SERVICE);
  damgr_get_tasks_from_packages_diff(queue, old_module.packages,
                                     module.packages);
  damgr_get_tasks_from_packages_diff(queue, old_module.aur_packages,
                                     module.aur_packages);
  damgr_get_tasks_from_hooks_diff(queue, old_module.pre_root_hooks,
                                  module.pre_root_hooks, PRE_ROOT_HOOK);
  damgr_get_tasks_from_hooks_diff(queue, old_module.pre_user_hooks,
                                  module.pre_user_hooks, PRE_USER_HOOK);
  damgr_get_tasks_from_hooks_diff(queue, old_module.post_root_hooks,
                                  module.post_root_hooks, POST_ROOT_HOOK);
  damgr_get_tasks_from_hooks_diff(queue, old_module.post_user_hooks,
                                  module.post_user_hooks, POST_USER_HOOK);
}

static void damgr_get_tasks_from_hosts_diff(Damgr_Tasks *tasks,
                                            Damgr_Host *old_host,
                                            Damgr_Host *host) {
  Damgr_Task_Queue service_queue = {.type = SERVICE,
                                    .queue_name = host->name,
                                    .owner_ptr = &host->root_services};
  damgr_get_tasks_from_root_services_diff(&service_queue,
                                          old_host->root_services,
                                          host->root_services, ROOT_SERVICE);
  if (service_queue.count > 0) {
    damgr_log(INFO, "successfully got %zu tasks for host: %s",
              service_queue.count, host->name);
    damgr_tasks_append(tasks, service_queue);
  }
  for (size_t i = 0; i < host->modules.count; ++i) {
    Damgr_Module *module = &host->modules.items[i];
    Damgr_Task_Queue module_queue = {
        .type = MODULE, .queue_name = module->name, .owner_ptr = module};
    for (size_t j = 0; j < old_host->modules.count; ++j) {
      Damgr_Module *old_module = &old_host->modules.items[j];
      // first check if the name lengths are equal, if so perform needle in
      // haystack search, else skip
      if (strlen(old_module->name) == strlen(module->name) &&
          damgr_string_contains(old_module->name, module->name)) {
        old_module->is_orphan =
            false; // to remove later flip the old module state
        damgr_get_tasks_from_module_diff(&module_queue, *old_module, *module);
        module->is_compared = true; // to skip later flip the new module state
        if (module_queue.count > 0) {
          damgr_log(
              INFO,
              "successfully got %zu tasks after comparison for module: %s",
              module_queue.count, module->name);
          damgr_tasks_append(tasks, module_queue);
        }
        break; // always break after diff calculation
      }
    }
    if (!module->is_compared) {
      get_tasks_from_module(&module_queue, *module, true);
      if (module_queue.count > 0) {
        damgr_log(INFO, "successfully got %zu tasks for new module: %s",
                  module_queue.count, module->name);
        damgr_tasks_append(tasks, module_queue);
      }
    }
  }
  // old modules need cleanup (is_new_state=false)
  for (size_t i = 0; i < old_host->modules.count; ++i) {
    Damgr_Module *old_module = &old_host->modules.items[i];
    if (old_module->is_orphan) {
      Damgr_Task_Queue module_queue = {.type = MODULE,
                                       .queue_name = old_module->name,
                                       .owner_ptr =
                                           &old_host->modules.items[i]};
      get_tasks_from_module(&module_queue, *old_module, false);
      if (module_queue.count > 0) {
        damgr_log(INFO, "successfully got %zu tasks for orphan module: %s",
                  module_queue.count, old_host->modules.items[i].name);
        damgr_tasks_append(tasks, module_queue);
      }
    }
  }
}

static void damgr_get_tasks_from_host(Damgr_Tasks *tasks, Damgr_Host *host) {
  Damgr_Task_Queue service_queue = {.type = SERVICE,
                                    .queue_name = host->name,
                                    .owner_ptr = &host->root_services};
  for (size_t i = 0; i < host->root_services.count; i++) {
    char *service = host->root_services.items[i];
    struct payload payload = {.payload_name = service};
    damgr_get_task(&service_queue, ROOT_SERVICE, true, payload);
  }
  if (service_queue.count > 0) {
    damgr_log(INFO, "successfully got %zu tasks for host: %s",
              service_queue.count, host->name);
    damgr_tasks_append(tasks, service_queue);
  }
  for (size_t i = 0; i < host->modules.count; i++) {
    Damgr_Module *module = &host->modules.items[i];
    Damgr_Task_Queue module_queue = {.type = MODULE,
                                     .queue_name = module->name,
                                     .owner_ptr = &host->modules.items[i]};
    get_tasks_from_module(&module_queue, *module, true);
    if (module_queue.count > 0) {
      damgr_log(INFO, "successfully got %zu tasks for module: %s",
                module_queue.count, host->modules.items[i].name);
      damgr_tasks_append(tasks, module_queue);
    }
  }
}

// TODO: host services can depend on packages?
int damgr_get_tasks(Damgr_Tasks *tasks, Damgr_Config *old_config,
                    Damgr_Config *config) {
  if (old_config->active_host.name != nullptr) {
    int ret = strcmp(old_config->active_host.name, config->active_host.name);
    if (ret < 0 || ret > 0) { // different host
      damgr_get_tasks_from_host(tasks, &config->active_host);
      return EXIT_SUCCESS;
    } else { // same host
      damgr_get_tasks_from_hosts_diff(tasks, &old_config->active_host,
                                      &config->active_host);
      return EXIT_SUCCESS;
    }
  } else { // no state host
    damgr_get_tasks_from_host(tasks, &config->active_host);
    return EXIT_SUCCESS;
  }
  damgr_log(ERROR, "failed to get tasks for the active host: %s",
            config->active_host);
  return EXIT_FAILURE;
}

static void damgr_do_task(Damgr_Task *task, char *aur_helper, char *user) {
  bool privileged;
  switch (task->type) {
  case PACKAGE:
  case AUR_PACKAGE:
    int ret;
    if (task->is_new_state) {
      if (task->type == PACKAGE) {
        ret = damgr_execute_package_install_command(task->payload.packages);
      } else {
        ret = damgr_execute_aur_package_install_command(task->payload.packages,
                                                        aur_helper);
      }
    } else {
      // !is_new_state
      ret = damgr_execute_package_remove_command(task->payload.packages);
    }
    task->status = ret == EXIT_SUCCESS ? SUCCEEDED : FAILED;
    break;

  case ROOT_SERVICE:
  case USER_SERVICE:
    privileged = (task->type == ROOT_SERVICE) ? true : false;
    task->status = damgr_execute_service_command(privileged, task->is_new_state,
                                                 task->payload.payload_name) ==
                           EXIT_SUCCESS
                       ? SUCCEEDED
                       : FAILED;
    break;

  case DOTFILE:
    task->status = damgr_execute_dotfile_command(user, task->is_new_state,
                                                 task->payload.payload_name) ==
                           EXIT_SUCCESS
                       ? SUCCEEDED
                       : FAILED;
    break;

  case PRE_ROOT_HOOK:
  case PRE_USER_HOOK:
  case POST_ROOT_HOOK:
  case POST_USER_HOOK:
    privileged = (task->type == PRE_ROOT_HOOK || task->type == POST_ROOT_HOOK)
                     ? true
                     : false;
    task->status =
        damgr_execute_hook_command(user, privileged,
                                   task->payload.payload_name) == EXIT_SUCCESS
            ? SUCCEEDED
            : FAILED;
    break;
  }
}

static void damgr_undo_task(Damgr_Task *task, char *user) {
  int ret;
  switch (task->type) {
  case PACKAGE:
  case AUR_PACKAGE:
    ret = damgr_execute_package_remove_command(task->payload.packages);
    break;

  case ROOT_SERVICE:
  case USER_SERVICE:
    bool privileged = (task->type == ROOT_SERVICE) ? true : false;
    ret = damgr_execute_service_command(privileged, false,
                                        task->payload.payload_name);
    break;

  case DOTFILE:
    ret =
        damgr_execute_dotfile_command(user, false, task->payload.payload_name);
    break;

  case PRE_ROOT_HOOK:
  case PRE_USER_HOOK:
  case POST_ROOT_HOOK:
  case POST_USER_HOOK:
    // CANT UNDO HOOKS HERE?
    ret = EXIT_SUCCESS;
    task->status = PENDING;
    break;
  }

  if (ret == EXIT_SUCCESS) {
    task->status = PENDING;
  }
}

static int queue_transaction(Damgr_Task_Queue queue, char *aur_helper,
                             char *user) {
  damgr_log(INFO, "%s queue transaction started...", queue.queue_name);
  bool failed = false;
  size_t i = 0;
  for (; i < queue.count; ++i) {
    Damgr_Task *task = &queue.items[i];
    if (task->status == PENDING) {
      damgr_log(INFO, "%s queue do task: %s", queue.queue_name,
                damgr_task_type_keys[task->type]);
      damgr_do_task(task, aur_helper, user);
      if (task->status == SUCCEEDED) {
        damgr_log(INFO, "%s queue task: %s succeeded!", queue.queue_name,
                  damgr_task_type_keys[task->type]);
      } else {
        damgr_log(ERROR, "%s queue task: %s failed!", queue.queue_name,
                  damgr_task_type_keys[task->type]);
        failed = true;
        break; // breaks the current for loop for rollback
      }
    }
  }

  if (failed) {
    damgr_log(INFO, "%s queue rollback started...", queue.queue_name);
    while (i > 0) {
      --i; // skip the last task because it failed no need for rollback
      Damgr_Task *task = &queue.items[i];
      damgr_undo_task(task, user);
      if (task->status == PENDING) {
        damgr_log(INFO, "%s queue task: %s rolled back!", queue.queue_name,
                  damgr_task_type_keys[task->type]);
      } else {
        damgr_log(ERROR, "%s queue task: %s failed to roll back!",
                  queue.queue_name, damgr_task_type_keys[task->type]);
      }
    }

    damgr_log(INFO, "%s queue rollback finished!", queue.queue_name);
    if (queue.type == SERVICE) {
      Damgr_Root_Services *owner_ptr = queue.owner_ptr;
      owner_ptr->to_write = false;
    } else {
      Damgr_Module *owner_ptr = queue.owner_ptr;
      owner_ptr->to_write = false;
    }
    return EXIT_FAILURE;
  }

  damgr_log(INFO, "%s queue transaction finished!", queue.queue_name);
  if (queue.type == SERVICE) {
    Damgr_Root_Services *owner_ptr = queue.owner_ptr;
    owner_ptr->to_write = true;
  } else {
    Damgr_Module *owner_ptr = queue.owner_ptr;
    owner_ptr->to_write = true;
  }
  return EXIT_SUCCESS;
}

int damgr_do_tasks(Damgr_Tasks tasks, char *aur_helper, char *user) {
  for (size_t i = 0; i < tasks.count; ++i) {
    if (queue_transaction(tasks.queues[i], aur_helper, user) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

void damgr_free_tasks(Damgr_Tasks *tasks) {
  for (size_t i = 0; i < tasks->count; ++i) {
    free(tasks->queues[i].items); // free the items buffer itself
  }
  free(tasks->queues); // free queues buffer itself
}
