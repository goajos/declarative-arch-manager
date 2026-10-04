#include "damgr/tasks.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <string.h>

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
    queue->tasks =
        realloc(queue->tasks, queue->capacity * sizeof(*queue->tasks));
  }
  queue->tasks[queue->count++] = task;
}

static void damgr_queues_append(Damgr_Task_Queues *queues,
                                Damgr_Task_Queue queue) {
  if (queues->count >= queues->capacity) {
    if (queues->capacity == 0) {
      queues->capacity = 16;
    } else {
      queues->capacity *= 2;
    }
    queues->queues =
        realloc(queues->queues, queues->capacity * sizeof(*queues->queues));
  }
  queues->queues[queues->count++] = queue;
}

static void damgr_get_task(Damgr_Task_Queue *queue, Damgr_Task_Type type,
                           bool is_positive, Damgr_Task_Payload payload) {
  Damgr_Task task = {
      .payload = payload, .type = type, .is_positive = is_positive};
  damgr_queue_append(queue, task);
}

static void damgr_get_tasks_from_services_diff(Damgr_Task_Queue *queue,
                                               Damgr_Darray old_services,
                                               Damgr_Darray services,
                                               Damgr_Task_Type type) {
  Damgr_Darray to_enable = {};
  Damgr_Darray to_disable = {};
  damgr_compute_darray_diff(&to_disable, &to_enable, old_services, services);
  for (size_t i = 0; i < to_enable.count; ++i) {
    char *service = to_enable.items[i];
    Damgr_Task_Payload payload = {.payload_name = service};
    damgr_get_task(queue, type, true, payload);
  }
  for (size_t i = 0; i < to_disable.count; ++i) {
    char *service = to_disable.items[i];
    Damgr_Task_Payload payload = {.payload_name = service};
    damgr_get_task(queue, type, false, payload);
  }
}

static void damgr_get_tasks_from_packages_diff(Damgr_Task_Queue *queue,
                                               Damgr_Darray old_packages,
                                               Damgr_Darray packages) {
  Damgr_Darray to_install = {};
  Damgr_Darray to_remove = {};
  damgr_compute_darray_diff(&to_install, &to_remove, old_packages, packages);
  if (to_install.count > 0) {
    Damgr_Task_Payload payload = {.packages = to_install};
    damgr_get_task(queue, PACKAGE, true, payload);
  }
  if (to_remove.count > 0) {
    Damgr_Task_Payload payload = {.packages = to_remove};
    damgr_get_task(queue, PACKAGE, false, payload);
  }
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
    Damgr_Task_Payload payload = {.payload_name = hook};
    damgr_get_task(queue, type, true, payload);
  }
}

static void damgr_get_tasks_from_dotfiles_diff(Damgr_Task_Queue *queue,
                                               bool to_link, bool old_to_link,
                                               char *module_name) {
  bool link;
  if (to_link && !old_to_link) {
    link = true;
  } else if (!to_link && old_to_link) {
    link = false;
  } else {
    return; // no need for (un)linking
  }
  Damgr_Task_Payload payload = {.payload_name = module_name};
  damgr_get_task(queue, DOTFILE, link, payload);
}

static void damgr_get_task_queue_from_module_diff(Damgr_Task_Queue *queue,
                                                  Damgr_Module old_module,
                                                  Damgr_Module module) {
  damgr_get_tasks_from_packages_diff(queue, old_module.packages,
                                     module.packages);
  damgr_get_tasks_from_packages_diff(queue, old_module.aur_packages,
                                     module.aur_packages);
  damgr_get_tasks_from_services_diff(queue, old_module.root_services,
                                     module.root_services, ROOT_SERVICE);
  damgr_get_tasks_from_services_diff(queue, old_module.user_services,
                                     module.user_services, USER_SERVICE);
  // pre hooks
  damgr_get_tasks_from_hooks_diff(queue, old_module.pre_root_hooks,
                                  module.pre_root_hooks, PRE_ROOT_HOOK);
  damgr_get_tasks_from_hooks_diff(queue, old_module.pre_user_hooks,
                                  module.pre_user_hooks, PRE_USER_HOOK);
  // post hooks
  damgr_get_tasks_from_hooks_diff(queue, old_module.post_root_hooks,
                                  module.post_root_hooks, POST_ROOT_HOOK);
  damgr_get_tasks_from_hooks_diff(queue, old_module.post_user_hooks,
                                  module.post_user_hooks, POST_USER_HOOK);
  damgr_get_tasks_from_dotfiles_diff(queue, module.to_link, old_module.to_link,
                                     module.module_name);
}

static void get_task_queue_from_module(Damgr_Task_Queue *queue,
                                       Damgr_Module module, bool is_positive) {
  // can't undo hooks
  if (is_positive) {
    for (size_t i = 0; i < module.pre_root_hooks.count; ++i) {
      char *hook = module.pre_root_hooks.items[i];
      Damgr_Task_Payload payload = {.payload_name = hook};
      damgr_get_task(queue, PRE_ROOT_HOOK, is_positive, payload);
    }
    for (size_t i = 0; i < module.pre_user_hooks.count; ++i) {
      char *hook = module.pre_user_hooks.items[i];
      Damgr_Task_Payload payload = {.payload_name = hook};
      damgr_get_task(queue, PRE_USER_HOOK, is_positive, payload);
    }
    for (size_t i = 0; i < module.post_root_hooks.count; ++i) {
      char *hook = module.post_root_hooks.items[i];
      Damgr_Task_Payload payload = {.payload_name = hook};
      damgr_get_task(queue, POST_ROOT_HOOK, is_positive, payload);
    }
    for (size_t i = 0; i < module.post_user_hooks.count; ++i) {
      char *hook = module.post_user_hooks.items[i];
      Damgr_Task_Payload payload = {.payload_name = hook};
      damgr_get_task(queue, POST_ROOT_HOOK, is_positive, payload);
    }
  }

  if (module.packages.count > 0) {
    Damgr_Task_Payload payload = {.packages = module.packages};
    damgr_get_task(queue, PACKAGE, is_positive, payload);
  }
  if (module.aur_packages.count > 0) {
    Damgr_Task_Payload payload = {.packages = module.aur_packages};
    damgr_get_task(queue, AUR_PACKAGE, is_positive, payload);
  }
  for (size_t i = 0; i < module.root_services.count; ++i) {
    char *service = module.root_services.items[i];
    Damgr_Task_Payload payload = {.payload_name = service};
    damgr_get_task(queue, ROOT_SERVICE, is_positive, payload);
  }
  for (size_t i = 0; i < module.user_services.count; ++i) {
    char *service = module.user_services.items[i];
    Damgr_Task_Payload payload = {.payload_name = service};
    damgr_get_task(queue, USER_SERVICE, is_positive, payload);
  }
  if (module.to_link) {
    Damgr_Task_Payload payload = {.payload_name = module.module_name};
    damgr_get_task(queue, DOTFILE, is_positive, payload);
  }
}

static void damgr_get_task_queues_from_hosts_diff(Damgr_Host *old_host,
                                                  Damgr_Host *host) {
  bool *is_orphan = malloc(old_host->modules.count * sizeof(bool));
  for (size_t i = 0; i < host->modules.count; ++i) {
    Damgr_Module *module = &host->modules.modules[i];
    Damgr_Task_Queue queue = {.module_ptr = nullptr, .status = PENDING};
    for (size_t j = 0; j < old_host->modules.count; ++j) {
      Damgr_Module *old_module = &old_host->modules.modules[j];
      // first check if the name lengths are equal, if so perform needle in
      // haystack search, else skip
      if (strlen(old_module->module_name) == strlen(module->module_name) &&
          damgr_string_contains(old_module->module_name, module->module_name)) {
        queue.module_ptr = module;
        is_orphan[j] = false;
        damgr_get_task_queue_from_module_diff(&queue, *old_module, *module);
        if (queue.count > 0) {
          damgr_log(
              INFO,
              "successfully got %zu tasks after comparison for module: %s",
              queue.count, module->module_name);
          damgr_queues_append(&host->task_queues, queue);
        }
        break; // always break after diff calculation
      }
    }
    if (queue.module_ptr == nullptr) {
      // no old module matched, so the module is new
      queue.module_ptr = module;
      get_task_queue_from_module(&queue, *module, true);
      if (queue.count > 0) {
        damgr_log(INFO, "successfully got %zu tasks for new module: %s",
                  queue.count, module->module_name);
        damgr_queues_append(&host->task_queues, queue);
      }
    }
  }
  // old modules need cleanup
  for (size_t i = 0; i < old_host->modules.count; ++i) {
    Damgr_Module *old_module = &old_host->modules.modules[i];
    if (is_orphan[i]) {
      Damgr_Task_Queue queue = {.module_ptr = &old_host->modules.modules[i],
                                .status = PENDING};
      get_task_queue_from_module(&queue, *old_module, false);
      if (queue.count > 0) {
        damgr_log(INFO, "successfully got %zu tasks for old module: %s",
                  queue.count, old_host->modules.modules[i].module_name);
        damgr_queues_append(&old_host->task_queues, queue);
      }
    }
  }
  free(is_orphan);
}

static void damgr_get_task_queues_from_host(Damgr_Host *host,
                                            bool is_positive) {
  for (size_t i = 0; i < host->modules.count; i++) {
    Damgr_Module *module = &host->modules.modules[i];
    Damgr_Task_Queue queue = {.module_ptr = &host->modules.modules[i],
                              .status = PENDING};
    get_task_queue_from_module(&queue, *module, is_positive);
    if (queue.count > 0) {
      damgr_log(INFO, "successfully got %zu tasks for module: %s", queue.count,
                host->modules.modules[i].module_name);
      damgr_queues_append(&host->task_queues, queue);
    }
  }
}

void damgr_get_task_queues_from_configs(Damgr_Config *old_config,
                                        Damgr_Config *config) {
  if (old_config->active_host.host_name != nullptr) {
    config->active_host.task_queues.host_ptr = &config->active_host;
    old_config->active_host.task_queues.host_ptr = &old_config->active_host;
    int ret = strcmp(old_config->active_host.host_name,
                     config->active_host.host_name);
    if (ret < 0 || ret > 0) { // different host
      damgr_get_task_queues_from_host(&config->active_host, true);
      // old host needs cleanup
      damgr_get_task_queues_from_host(&old_config->active_host, false);
      goto exit;
    } else { // same host
      damgr_get_task_queues_from_hosts_diff(&old_config->active_host,
                                            &config->active_host);
      goto exit;
    }
  } else { // no state host, old_config->active_host.host_name == nullptr
    config->active_host.task_queues.host_ptr = &config->active_host;
    damgr_get_task_queues_from_host(&config->active_host, true);
    goto exit;
  }
  return;
exit:
  if (config->active_host.task_queues.count > 0) {
    damgr_log(INFO, "successfully got %zu task queues for host: %s",
              config->active_host.task_queues.count,
              config->active_host.host_name);
  }
  if (old_config->active_host.task_queues.count > 0) {
    damgr_log(INFO, "successfully got %zu task queues for old host: %s",
              old_config->active_host.task_queues.count,
              old_config->active_host.host_name);
  }
}

static void task_queue_transaction(Damgr_Task_Queue *queue,
                                   [[maybe_unused]] char *aur_helper) {
  damgr_log(INFO, "%s task queue transaction started...",
            queue->module_ptr->module_name);
  [[maybe_unused]] bool failed = false;
  [[maybe_unused]] size_t i = 0;
  for (; queue->count; ++i) {
  }
}

void damgr_do_task_queues_for_config(Damgr_Config *config, bool is_positive) {
  damgr_log(INFO, "starting task queue transactions for host %s",
            config->active_host.host_name);
  for (size_t i = 0; i < config->active_host.task_queues.count; ++i) {
    Damgr_Task_Queue *queue = &config->active_host.task_queues.queues[i];
    if (queue->status == PENDING) {
      task_queue_transaction(queue, config->aur_helper);
      if (queue->status == SUCCEEDED) {
        if (is_positive) {
          // TODO: write module to state
        } else {
          // TODO: remove old module from state
        }
      } else if (queue->status == FAILED) {
        // TODO: report failure?
      }
    }
  }
  if (is_positive) {
    // TODO: write host to state
  } else {
    // TODO: remove old host from state
  }
}
