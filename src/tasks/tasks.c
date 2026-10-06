#include "damgr/tasks.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include <string.h>

Damgr_Task *task_constructor(Damgr_Task_Payload payload, Damgr_Task_Type type,
                             bool is_positive) {
  Damgr_Task *task = malloc(sizeof(*task));
  task->payload = payload;
  task->type = type;
  task->is_positive = is_positive;
  return task;
}

Damgr_Task_Queue *task_queue_constructor(Damgr_Status status) {
  Damgr_Task_Queue *task_queue = malloc(sizeof(*task_queue));
  task_queue->status = status;
  return task_queue;
}

static void damgr_compute_darray_diff(Damgr_Darray *negative,
                                      Damgr_Darray *positive,
                                      Damgr_Darray old_array,
                                      Damgr_Darray array) {
  qsort(old_array.ptrs, old_array.count, sizeof(old_array.ptrs[0]),
        damgr_qcharcmp);
  qsort(array.ptrs, array.count, sizeof(array.ptrs[0]), damgr_qcharcmp);
  size_t i = 0;
  size_t j = 0;
  while (i < old_array.count && j < array.count) {
    int ret = strcmp(old_array.ptrs[i], array.ptrs[j]);
    if (ret < 0) {
      // hooks passes nullptr for negativbe darray
      if (negative != nullptr) {
        damgr_darray_append(negative, strdup(old_array.ptrs[i]));
      }
      ++i;
    } else if (ret > 0) {
      damgr_darray_append(positive, array.ptrs[j]);
      ++j;
    } else {
      ++i;
      ++j;
    }
  }
  while (i < old_array.count) {
    // hooks passes nullptr for negativbe darray
    if (negative != nullptr) {
      damgr_darray_append(negative, strdup(old_array.ptrs[i]));
    }
    ++i;
  }
  while (j < array.count) {
    damgr_darray_append(positive, array.ptrs[j]);
    ++j;
  }
}

static void damgr_get_tasks_from_services_diff(Damgr_Task_Queue *queue,
                                               Damgr_Darray old_services,
                                               Damgr_Darray services,
                                               Damgr_Task_Type type) {
  Damgr_Darray to_enable = {};
  Damgr_Darray to_disable = {};
  damgr_compute_darray_diff(&to_disable, &to_enable, old_services, services);
  for (size_t i = 0; i < to_enable.count; ++i) {
    Damgr_Task_Payload payload = {.payload_name = to_enable.ptrs[i]};
    Damgr_Task *task = task_constructor(payload, type, true);
    damgr_darray_append(&queue->tasks, task);
  }
  for (size_t i = 0; i < to_disable.count; ++i) {
    Damgr_Task_Payload payload = {.payload_name = to_disable.ptrs[i]};
    Damgr_Task *task = task_constructor(payload, type, false);
    damgr_darray_append(&queue->tasks, task);
  }
}

static void damgr_get_tasks_from_packages_diff(Damgr_Task_Queue *queue,
                                               Damgr_Darray old_packages,
                                               Damgr_Darray packages,
                                               Damgr_Task_Type type) {
  Damgr_Darray to_install = {};
  Damgr_Darray to_remove = {};
  damgr_compute_darray_diff(&to_remove, &to_install, old_packages, packages);
  if (to_install.count > 0) {
    Damgr_Task_Payload payload = {.packages = to_install};
    Damgr_Task *task = task_constructor(payload, type, true);
    damgr_darray_append(&queue->tasks, task);
  }
  if (to_remove.count > 0) {
    Damgr_Task_Payload payload = {.packages = to_remove};
    Damgr_Task *task = task_constructor(payload, type, false);
    damgr_darray_append(&queue->tasks, task);
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
    Damgr_Task_Payload payload = {.payload_name = to_run.ptrs[i]};
    Damgr_Task *task = task_constructor(payload, type, true);
    damgr_darray_append(&queue->tasks, task);
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
  Damgr_Task *task = task_constructor(payload, DOTFILE, link);
  damgr_darray_append(&queue->tasks, task);
}

static void damgr_get_task_queue_from_module_diff(Damgr_Task_Queue *queue,
                                                  Damgr_Module *old_module,
                                                  Damgr_Module *module) {
  damgr_get_tasks_from_packages_diff(queue, old_module->packages,
                                     module->packages, PACKAGE);
  damgr_get_tasks_from_packages_diff(queue, old_module->aur_packages,
                                     module->aur_packages, AUR_PACKAGE);
  damgr_get_tasks_from_services_diff(queue, old_module->root_services,
                                     module->root_services, ROOT_SERVICE);
  damgr_get_tasks_from_services_diff(queue, old_module->user_services,
                                     module->user_services, USER_SERVICE);
  damgr_get_tasks_from_dotfiles_diff(queue, module->to_link,
                                     old_module->to_link, module->module_name);
  // pre hooks
  damgr_get_tasks_from_hooks_diff(queue, old_module->pre_root_hooks,
                                  module->pre_root_hooks, PRE_ROOT_HOOK);
  damgr_get_tasks_from_hooks_diff(queue, old_module->pre_user_hooks,
                                  module->pre_user_hooks, PRE_USER_HOOK);
  // post hooks
  damgr_get_tasks_from_hooks_diff(queue, old_module->post_root_hooks,
                                  module->post_root_hooks, POST_ROOT_HOOK);
  damgr_get_tasks_from_hooks_diff(queue, old_module->post_user_hooks,
                                  module->post_user_hooks, POST_USER_HOOK);
}

static void get_task_queue_from_module(Damgr_Task_Queue *queue,
                                       Damgr_Module *module) {
  // can't undo hooks
  bool is_positive =
      !module->is_state; // state == true equals is_positive == false
  if (is_positive) {
    for (size_t i = 0; i < module->pre_root_hooks.count; ++i) {
      char *hook = module->pre_root_hooks.ptrs[i];
      Damgr_Task_Payload payload = {.payload_name = hook};
      Damgr_Task *task = task_constructor(payload, PRE_ROOT_HOOK, is_positive);
      damgr_darray_append(&queue->tasks, task);
    }
    for (size_t i = 0; i < module->pre_user_hooks.count; ++i) {
      char *hook = module->pre_user_hooks.ptrs[i];
      Damgr_Task_Payload payload = {.payload_name = hook};
      Damgr_Task *task = task_constructor(payload, PRE_USER_HOOK, is_positive);
      damgr_darray_append(&queue->tasks, task);
    }
    for (size_t i = 0; i < module->post_root_hooks.count; ++i) {
      char *hook = module->post_root_hooks.ptrs[i];
      Damgr_Task_Payload payload = {.payload_name = hook};
      Damgr_Task *task = task_constructor(payload, POST_ROOT_HOOK, is_positive);
      damgr_darray_append(&queue->tasks, task);
    }
    for (size_t i = 0; i < module->post_user_hooks.count; ++i) {
      char *hook = module->post_user_hooks.ptrs[i];
      Damgr_Task_Payload payload = {.payload_name = hook};
      Damgr_Task *task = task_constructor(payload, POST_USER_HOOK, is_positive);
      damgr_darray_append(&queue->tasks, task);
    }
  }

  if (module->packages.count > 0) {
    Damgr_Task_Payload payload = {.packages = module->packages};
    Damgr_Task *task = task_constructor(payload, PACKAGE, is_positive);
    damgr_darray_append(&queue->tasks, task);
  }
  if (module->aur_packages.count > 0) {
    Damgr_Task_Payload payload = {.packages = module->aur_packages};
    Damgr_Task *task = task_constructor(payload, AUR_PACKAGE, is_positive);
    damgr_darray_append(&queue->tasks, task);
  }
  for (size_t i = 0; i < module->root_services.count; ++i) {
    char *service = module->root_services.ptrs[i];
    Damgr_Task_Payload payload = {.payload_name = service};
    Damgr_Task *task = task_constructor(payload, ROOT_SERVICE, is_positive);
    damgr_darray_append(&queue->tasks, task);
  }
  for (size_t i = 0; i < module->user_services.count; ++i) {
    char *service = module->user_services.ptrs[i];
    Damgr_Task_Payload payload = {.payload_name = service};
    Damgr_Task *task = task_constructor(payload, USER_SERVICE, is_positive);
    damgr_darray_append(&queue->tasks, task);
  }
  if (module->to_link) {
    Damgr_Task_Payload payload = {.payload_name = module->module_name};
    Damgr_Task *task = task_constructor(payload, DOTFILE, is_positive);
    damgr_darray_append(&queue->tasks, task);
  }
}

static void damgr_get_task_queues_from_hosts_diff(Damgr_Host *old_host,
                                                  Damgr_Host *host) {
  for (size_t i = 0; i < host->modules.count; ++i) {
    Damgr_Task_Queue *queue = task_queue_constructor(PENDING);
    Damgr_Module *module = host->modules.ptrs[i];
    for (size_t j = 0; j < old_host->modules.count; ++j) {
      Damgr_Module *old_module = old_host->modules.ptrs[j];
      if (old_module == nullptr) {
        continue;
      }
      // first check if the name lengths are equal, if so perform needle in
      // haystack search, else skip
      if (strlen(old_module->module_name) == strlen(module->module_name) &&
          damgr_string_contains(old_module->module_name, module->module_name)) {
        damgr_get_task_queue_from_module_diff(queue, old_module, module);
        if (queue->tasks.count > 0) {
          damgr_log(
              INFO,
              "successfully got %zu tasks after comparison for module: %s",
              queue->tasks.count, module->module_name);
          damgr_darray_append(&host->task_queues, queue);
          module->state_path = strdup(old_module->state_path);
          damgr_free_module(old_module);
          old_host->modules.ptrs[j] = nullptr;
        }
        break; // always break after diff calculation
      }
    }
    if (module->state_path == nullptr) {
      // no old module matched, so the module is new
      get_task_queue_from_module(queue, module);
      if (queue->tasks.count > 0) {
        damgr_log(INFO, "successfully got %zu tasks for new module: %s",
                  queue->tasks.count, module->module_name);
        damgr_darray_append(&host->task_queues, queue);
      } else {
        free(queue); // no tasks after comparison and new
      }
    }
  }
  // old modules need cleanup
  for (size_t i = 0; i < old_host->modules.count; ++i) {
    Damgr_Module *old_module = old_host->modules.ptrs[i];
    if (old_module != nullptr) {
      Damgr_Task_Queue *queue = task_queue_constructor(PENDING);
      get_task_queue_from_module(queue, old_module);
      if (queue->tasks.count > 0) {
        damgr_log(INFO, "successfully got %zu tasks for old module: %s",
                  queue->tasks.count, old_module->module_name);
        damgr_darray_append(&old_host->task_queues, queue);
      } else {
        free(queue);
      }
    }
  }
}

static void damgr_get_task_queues_from_host(Damgr_Host *host) {
  for (size_t i = 0; i < host->modules.count; i++) {
    Damgr_Module *module = host->modules.ptrs[i];
    Damgr_Task_Queue *queue = task_queue_constructor(PENDING);
    get_task_queue_from_module(queue, module);
    if (queue->tasks.count > 0) {
      damgr_log(INFO, "successfully got %zu tasks for module: %s",
                queue->tasks.count, module->module_name);
      damgr_darray_append(&host->task_queues, queue);
    } else {
      free(queue);
    }
  }
}

void damgr_get_task_queues_from_configs(Damgr_Config *old_config,
                                        Damgr_Config *config) {
  if (old_config->active_host != nullptr) {
    int ret = strcmp(old_config->active_host->host_name,
                     config->active_host->host_name);
    if (ret < 0 || ret > 0) { // different host
      damgr_get_task_queues_from_host(config->active_host);
      damgr_get_task_queues_from_host(old_config->active_host);
      goto exit;
    } else { // same host
      damgr_get_task_queues_from_hosts_diff(old_config->active_host,
                                            config->active_host);
      goto exit;
    }
  } else { // no state host
    damgr_get_task_queues_from_host(config->active_host);
    goto exit;
  }
  return;
exit:
  if (config->active_host->task_queues.count > 0) {
    damgr_log(INFO, "successfully got %zu task queues for host: %s",
              config->active_host->task_queues.count,
              config->active_host->host_name);
  }
  if (old_config->active_host->task_queues.count > 0) {
    damgr_log(INFO, "successfully got %zu task queues for old host: %s",
              old_config->active_host->task_queues.count,
              old_config->active_host->host_name);
  }
}

// static void damgr_do_task(Damgr_Status *queue_status, Damgr_Task task,
//                           char *aur_helper) {
//   bool privileged;
//   switch (task.type) {
//   case PACKAGE:
//   case AUR_PACKAGE:
//     int ret;
//     if (task.is_positive) {
//       if (task.type == PACKAGE) {
//         ret = damgr_execute_package_install_command(task.payload.packages);
//       } else {
//         ret =
//         damgr_execute_aur_package_install_command(task.payload.packages,
//                                                         aur_helper);
//       }
//     } else {
//       // !is_positive
//       ret = damgr_execute_package_remove_command(task.payload.packages);
//     }
//     *queue_status = ret == EXIT_SUCCESS ? SUCCEEDED : FAILED;
//     break;
//   case ROOT_SERVICE:
//   case USER_SERVICE:
//     privileged = (task.type == ROOT_SERVICE) ? true : false;
//     *queue_status =
//         damgr_execute_service_command(privileged, task.is_positive,
//                                       task.payload.payload_name) ==
//                                       EXIT_SUCCESS
//             ? SUCCEEDED
//             : FAILED;
//     break;
//   case DOTFILE:
//     *queue_status =
//         damgr_execute_dotfile_command(task.is_positive,
//                                       task.payload.payload_name) ==
//                                       EXIT_SUCCESS
//             ? SUCCEEDED
//             : FAILED;
//     break;
//   case PRE_ROOT_HOOK:
//   case PRE_USER_HOOK:
//   case POST_ROOT_HOOK:
//   case POST_USER_HOOK:
//     privileged = (task.type == PRE_ROOT_HOOK || task.type == POST_ROOT_HOOK)
//                      ? true
//                      : false;
//     *queue_status = damgr_execute_hook_command(
//                         privileged, task.payload.payload_name) ==
//                         EXIT_SUCCESS ? SUCCEEDED : FAILED;
//     break;
//   }
// }

// static void damgr_undo_task(Damgr_Status *queue_status, Damgr_Task task) {
//   int ret;
//   switch (task.type) {
//   case PACKAGE:
//   case AUR_PACKAGE:
//     ret = damgr_execute_package_remove_command(task.payload.packages);
//     break;
//   case ROOT_SERVICE:
//   case USER_SERVICE:
//     bool privileged = (task.type == ROOT_SERVICE) ? true : false;
//     ret = damgr_execute_service_command(privileged, false,
//                                         task.payload.payload_name);
//     break;
//   case DOTFILE:
//     ret = damgr_execute_dotfile_command(false, task.payload.payload_name);
//     break;
//   case PRE_ROOT_HOOK:
//   case PRE_USER_HOOK:
//   case POST_ROOT_HOOK:
//   case POST_USER_HOOK:
//     // can't undo hooks
//     ret = EXIT_SUCCESS;
//     *queue_status = PENDING;
//     break;
//   }
//
//   if (ret == EXIT_SUCCESS) {
//     *queue_status = PENDING;
//   } else {
//     *queue_status = FAILED;
//   }
// }

// static void task_queue_transaction(Damgr_Task_Queue *queue, char *aur_helper)
// {
//   damgr_log(INFO, "%s task queue transaction started...",
//             queue->module_ptr->module_name);
//   size_t i = 0;
//   for (; queue->count; ++i) {
//     if (i == queue->count) {
//       return;
//     }
//     damgr_do_task(&queue->status, queue->tasks[i], aur_helper);
//     if (queue->status == FAILED) {
//       damgr_log(ERROR, "%s task queue transaction failed...",
//                 queue->module_ptr->module_name);
//       break;
//     }
//   }
//   // undo tasks after queue failure
//   if (queue->status == FAILED) {
//     damgr_log(INFO, "starting %s task queue rollback...",
//               queue->module_ptr->module_name);
//     while (i > 0) {
//       --i; // last task failed so no need for rollback
//       damgr_undo_task(&queue->status, queue->tasks[i]);
//       if (queue->status == FAILED) {
//         damgr_log(ERROR, "%s task queue rollback failed!",
//                   queue->module_ptr->module_name);
//         return;
//       }
//     }
//     damgr_log(INFO, "%s task queue rollback finished!",
//               queue->module_ptr->module_name);
//   }
// }

// void damgr_do_task_queues_for_config(Damgr_Config *config, bool is_positive)
// {
//   damgr_log(INFO, "starting task queue transactions for host %s",
//             config->active_host.host_name);
//   for (size_t i = 0; i < config->active_host.task_queues.count; ++i) {
//     Damgr_Task_Queue *queue = &config->active_host.task_queues.queues[i];
//     if (queue->status == PENDING) {
//       task_queue_transaction(queue, config->aur_helper);
//       if (queue->status == SUCCEEDED) {
//         if (is_positive) {
//           // config is_positive = true
//           damgr_write_module(*queue->module_ptr);
//         } else {
//           // old config is_positive = false
//           damgr_remove_module(*queue->module_ptr);
//         }
//       } else if (queue->status == FAILED) {
//         // TODO: report failure?
//         // to skip writing failed module to host state
//         queue->module_ptr->module_name = nullptr;
//       }
//     }
//   }
//   if (is_positive) {
//     // config is_positive = true
//     damgr_write_host(config->active_host);
//   } else {
//     // old config is_positive = false
//     if (config->active_host.task_queues.host_ptr->host_name != nullptr) {
//       // only remove the old host state if it's a different host
//       damgr_remove_host(config->active_host);
//     }
//   }
// }
