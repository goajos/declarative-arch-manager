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

// TODO: finish copying over these functions
static void damgr_get_tasks_from_module_diff(Damgr_Task_Queue *queue,
                                             Damgr_Module old_module,
                                             Damgr_Module module) {
  // damgr_get_tasks_from_services_diff(queue, old_module.user_services,
  //                                    module.user_services, USER_SERVICE);
  // damgr_get_tasks_from_packages_diff(queue, old_module.packages,
  //                                    module.packages);
  // damgr_get_tasks_from_packages_diff(queue, old_module.aur_packages,
  //                                    module.aur_packages);
  // damgr_get_tasks_from_hooks_diff(queue, old_module.pre_root_hooks,
  //                                 module.pre_root_hooks, PRE_ROOT_HOOK);
  // damgr_get_tasks_from_hooks_diff(queue, old_module.pre_user_hooks,
  //                                 module.pre_user_hooks, PRE_USER_HOOK);
  // damgr_get_tasks_from_hooks_diff(queue, old_module.post_root_hooks,
  //                                 module.post_root_hooks, POST_ROOT_HOOK);
  // damgr_get_tasks_from_hooks_diff(queue, old_module.post_user_hooks,
  //                                 module.post_user_hooks, POST_USER_HOOK);
}

static void get_tasks_from_module(Damgr_Task_Queue *queue, Damgr_Module module,
                                  bool is_positive) {
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
  }
  if (module.packages.count > 0) {
    Damgr_Task_Payload payload = {.packages = module.packages};
    damgr_get_task(queue, PACKAGE, is_positive, payload);
  }
  if (module.aur_packages.count > 0) {
    Damgr_Task_Payload payload = {.packages = module.aur_packages};
    damgr_get_task(queue, AUR_PACKAGE, is_positive, payload);
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
  if (is_positive) {
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
}

static void damgr_get_tasks_from_hosts_diff(Damgr_Task_Queues *queues,
                                            Damgr_Host *old_host,
                                            Damgr_Host *host) {
  bool *is_orphan = malloc(old_host->modules.count * sizeof(bool));
  for (size_t i = 0; i < host->modules.count; ++i) {
    Damgr_Module *module = &host->modules.modules[i];
    Damgr_Task_Queue queue = {.queue_name = module->module_name,
                              .module_ptr = &host->modules.modules[i],
                              .old_ptr = nullptr,
                              .status = PENDING};
    for (size_t j = 0; j < old_host->modules.count; ++j) {
      Damgr_Module *old_module = &old_host->modules.modules[j];
      // first check if the name lengths are equal, if so perform needle in
      // haystack search, else skip
      if (strlen(old_module->module_name) == strlen(module->module_name) &&
          damgr_string_contains(old_module->module_name, module->module_name)) {
        is_orphan[j] = false;
        queue.old_ptr = old_module;
        damgr_get_tasks_from_module_diff(&queue, *old_module, *module);
        if (queue.count > 0) {
          damgr_log(
              INFO,
              "successfully got %zu tasks after comparison for module: %s",
              queue.count, module->module_name);
          damgr_queues_append(queues, queue);
        }
        break; // always break after diff calculation
      }
    }
    if (queue.old_ptr == nullptr) {
      // no old module matched, so the module is new
      get_tasks_from_module(&queue, *module, true);
      if (queue.count > 0) {
        damgr_log(INFO, "successfully got %zu tasks for new module: %s",
                  queue.count, module->module_name);
        damgr_queues_append(queues, queue);
      }
    }
  }
  // old modules need cleanup
  for (size_t i = 0; i < old_host->modules.count; ++i) {
    Damgr_Module *old_module = &old_host->modules.modules[i];
    if (is_orphan[i]) {
      Damgr_Task_Queue queue = {.queue_name = old_module->module_name,
                                .module_ptr = &old_host->modules.modules[i],
                                .old_ptr = nullptr,
                                .status = PENDING};
      get_tasks_from_module(&queue, *old_module, false);
      if (queue.count > 0) {
        damgr_log(INFO, "successfully got %zu tasks for orphan module: %s",
                  queue.count, old_host->modules.modules[i].module_name);
        damgr_queues_append(queues, queue);
      }
    }
  }
  free(is_orphan);
}

static void damgr_get_tasks_from_host(Damgr_Task_Queues *queues,
                                      Damgr_Host *host) {
  for (size_t i = 0; i < host->modules.count; i++) {
    Damgr_Module *module = &host->modules.modules[i];
    Damgr_Task_Queue queue = {.queue_name = module->module_name,
                              .module_ptr = &host->modules.modules[i],
                              .old_ptr = nullptr,
                              .status = PENDING};
    get_tasks_from_module(&queue, *module, true);
    if (queue.count > 0) {
      damgr_log(INFO, "successfully got %zu tasks for module: %s", queue.count,
                host->modules.modules[i].module_name);
      damgr_queues_append(queues, queue);
    }
  }
}

void damgr_get_tasks(Damgr_Config *old_config, Damgr_Config *config) {
  Damgr_Task_Queues *queues = &config->active_host.task_queues;
  queues->host_ptr = &config->active_host;
  queues->old_ptr = nullptr;
  if (old_config->active_host.host_name != nullptr) {
    queues->old_ptr = &old_config->active_host; // flag for cleanup
    int ret = strcmp(old_config->active_host.host_name,
                     config->active_host.host_name);
    if (ret < 0 || ret > 0) { // different host
      damgr_get_tasks_from_host(queues, &config->active_host);
      goto exit;
    } else { // same host
      // TODO: finish the hosts diff for getting tasks!
      damgr_get_tasks_from_hosts_diff(queues, &old_config->active_host,
                                      &config->active_host);
    }
  } else { // no state host
    damgr_get_tasks_from_host(queues, &config->active_host);
    goto exit;
  }
  return;
exit:
  if (queues->count > 0) {
    damgr_log(INFO, "successfully got %zu task queues for host: %s",
              queues->count, config->active_host.host_name);
  }
}
