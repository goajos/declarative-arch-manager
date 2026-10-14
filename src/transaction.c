#include "damgr/transaction.h"
#include "damgr/log.h"
#include "damgr/state.h"

static const char *task_type_string(Damgr_Task_Type type) {
  switch (type) {
  case PACKAGE_INSTALL:
    return "PACKAGE_INSTALL";
  case PACKAGE_REMOVE:
    return "PACKAGE_REMOVE";
  case AUR_PACKAGE_INSTALL:
    return "AUR_PACKAGE_INSTALL";
  case AUR_PACKAGE_REMOVE:
    return "AUR_PACKAGE_REMOVE";
  case SERVICE_ENABLE:
    return "SERVICE_ENABLE";
  case SERVICE_DISABLE:
    return "SERVICE_DISABLE";
  case DOTFILE_LINK:
    return "DOTFILE_LINK";
  case DOTFILE_UNLINK:
    return "DOTFILE_UNLINK";
  }
}

// queue owns its own memory
// static int queue_push(Damgr_Task_Queue *queue, Damgr_Task_Type type,
//                       const char *module_name, const char *name,
//                       const char **packages, size_t packages_cnt,
//                       bool privileged) {
//   // grow queue dynamically
//   if (queue->cnt >= queue->capacity) {
//     if (queue->capacity == 0) {
//       queue->capacity = 32;
//     } else {
//       queue->capacity *= 2;
//     }
//     Damgr_Task *tasks =
//         realloc(queue->tasks, queue->capacity * sizeof(Damgr_Task));
//     if (tasks == nullptr) {
//       damgr_log(ERROR, "queue realloc failed and returned nullptr");
//       return EXIT_FAILURE;
//     }
//     queue->tasks = tasks;
//   }
//
//   Damgr_Task task = {0}; // zero init for name and packages
//   task.type = type;
//   task.privileged = privileged;
//   task.status = PENDING;
//   task.module_name = module_name != nullptr ? strdup(module_name) : nullptr;
//   if (module_name != nullptr && task.module_name == nullptr) {
//     damgr_log(ERROR, "task strdup failed and returned nullptr");
//     goto cleanup;
//   }
//   task.name = name != nullptr ? strdup(name) : nullptr;
//   if (name != nullptr && task.name == nullptr) {
//     damgr_log(ERROR, "task strdup failed and returned nullptr");
//     goto cleanup;
//   }
//   if (packages_cnt > 0) {
//     task.packages = calloc(packages_cnt, sizeof(char *));
//     if (task.packages == nullptr) {
//       damgr_log(ERROR, "task calloc failed and returned nullptr");
//       goto cleanup;
//     }
//     for (size_t i = 0; i < packages_cnt; ++i) {
//       task.packages[i] = strdup(packages[i]);
//       if (task.packages[i] == nullptr) {
//         damgr_log(ERROR, "task strdup failed and returned nullptr");
//         goto cleanup;
//       }
//     }
//     task.packages_cnt = packages_cnt;
//   }
//
//   queue->tasks[queue->cnt++] = task;
//   return EXIT_SUCCESS;
//
// cleanup:
//   if (task.module_name != nullptr)
//     free(task.module_name);
//   if (task.name != nullptr)
//     free(task.name);
//   for (size_t i = 0; i < packages_cnt; ++i) {
//     if (task.packages[i] != nullptr)
//       free(task.packages[i]);
//   }
//   if (task.packages != nullptr)
//     free(task.packages);
//   return EXIT_FAILURE;
// }

// void damgr_free_queue(Damgr_Task_Queue *queue) {
//   for (size_t i = 0; i < queue->cnt; ++i) {
//     free(queue->tasks[i].module_name);
//     free(queue->tasks[i].name);
//     for (size_t j = 0; j < queue->tasks[i].packages_cnt; ++j)
//       free(queue->tasks[i].packages[j]);
//     free(queue->tasks[i].packages);
//   }
//   free(queue->tasks);
//   queue->tasks = nullptr;
//   queue->cnt = 0;
//   queue->capacity = 0;
// }

// queue borrows its memory from the arena
static int queue_push(Damgr_Task_Queue *queue, Damgr_Task_Type type,
                      const char *module_name, const char *name,
                      const char **packages, size_t packages_cnt,
                      bool privileged) {
  // grow queue dynamically
  if (queue->cnt >= queue->capacity) {
    if (queue->capacity == 0) {
      queue->capacity = 32;
    } else {
      queue->capacity *= 2;
    }
    Damgr_Task *tasks =
        realloc(queue->tasks, queue->capacity * sizeof(Damgr_Task));
    if (tasks == nullptr) {
      damgr_log(ERROR, "queue realloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
    queue->tasks = tasks;
  }

  Damgr_Task task = {0}; // zero init for name and packages
  task.type = type;
  task.privileged = privileged;
  task.status = PENDING;
  task.module_name = module_name != nullptr ? module_name : nullptr;
  task.name = name != nullptr ? name : nullptr;
  task.packages = packages;
  task.packages_cnt = packages_cnt;

  queue->tasks[queue->cnt++] = task;
  return EXIT_SUCCESS;
}

static int add_package_tasks(Damgr_Array_Diff *array_diff,
                             Damgr_Task_Type install_type,
                             Damgr_Task_Type remove_type,
                             const char *module_name, Damgr_Task_Queue *queue,
                             Damgr_Arena *arena) {
  size_t install_cnt = 0;
  size_t remove_cnt = 0;
  for (size_t i = 0; i < array_diff->active_cnt; ++i)
    if (array_diff->only_in_active[i])
      ++install_cnt;
  for (size_t i = 0; i < array_diff->state_cnt; ++i)
    if (array_diff->only_in_state[i])
      ++remove_cnt;

  int ret = EXIT_SUCCESS;
  if (install_cnt > 0) {
    // const char **to_install = calloc(install_cnt, sizeof(char *));
    const char **to_install = damgr_arena_alloc(arena, install_cnt);
    if (to_install == nullptr) {
      damgr_log(ERROR, "package task calloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
    size_t j = 0;
    for (size_t i = 0; i < array_diff->active_cnt; ++i)
      if (array_diff->only_in_active[i])
        to_install[j++] = array_diff->active_array[i];
    ret = queue_push(queue, install_type, module_name, nullptr, to_install,
                     install_cnt, false);
    // free(to_install);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
  }
  if (remove_cnt > 0) {
    // const char **to_remove = calloc(remove_cnt, sizeof(char *));
    const char **to_remove = damgr_arena_alloc(arena, remove_cnt);
    if (to_remove == nullptr) {
      damgr_log(ERROR, "package task calloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
    size_t j = 0;
    for (size_t i = 0; i < array_diff->state_cnt; ++i)
      if (array_diff->only_in_state[i])
        to_remove[j++] = array_diff->state_array[i];
    ret = queue_push(queue, remove_type, module_name, nullptr, to_remove,
                     remove_cnt, false);
    // free(to_remove);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
  }
  return ret;
}

static int add_service_tasks(const Damgr_Array_Diff *array_diff,
                             const char *module_name, bool privileged,
                             Damgr_Task_Queue *queue) {
  int ret = EXIT_SUCCESS;
  for (size_t i = 0; i < array_diff->active_cnt; ++i) {
    if (array_diff->only_in_active[i]) {
      ret = queue_push(queue, SERVICE_ENABLE, module_name,
                       array_diff->active_array[i], nullptr, 0, privileged);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
  }
  for (size_t i = 0; i < array_diff->state_cnt; ++i) {
    if (array_diff->only_in_state[i]) {
      ret = queue_push(queue, SERVICE_DISABLE, module_name,
                       array_diff->state_array[i], nullptr, 0, privileged);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
  }
  return ret;
}

// static int add_module_tasks(Damgr_Module_Diff *module_diff,
//                             Damgr_Task_Queue *queue) {
static int add_module_tasks(Damgr_Module_Diff *module_diff,
                            Damgr_Task_Queue *queue, Damgr_Arena *arena) {
  int ret = EXIT_SUCCESS;
  switch (module_diff->diff_type) {
  case SKIP:
    break;
  case ADD: {
    const Damgr_Module *module = module_diff->active_module;
    if (module->packages_cnt > 0) {
      ret = queue_push(queue, PACKAGE_INSTALL, module->module_name, nullptr,
                       module->packages, module->packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->aur_packages_cnt > 0) {
      ret = queue_push(queue, AUR_PACKAGE_INSTALL, module->module_name, nullptr,
                       module->aur_packages, module->aur_packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < module->root_services_cnt; ++i) {
      ret = queue_push(queue, SERVICE_ENABLE, module->module_name,
                       module->root_services[i], nullptr, 0, true);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < module->user_services_cnt; ++i) {
      ret = queue_push(queue, SERVICE_ENABLE, module->module_name,
                       module->user_services[i], nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->to_link) {
      ret = queue_push(queue, DOTFILE_LINK, module->module_name,
                       module->module_name, nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    break;
  }
  case CHANGE: {
    const Damgr_Module *module = module_diff->active_module;
    ret = add_package_tasks(&module_diff->packages, PACKAGE_INSTALL,
                            PACKAGE_REMOVE, module->module_name, queue, arena);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
    ret = add_package_tasks(&module_diff->aur_packages, AUR_PACKAGE_INSTALL,
                            AUR_PACKAGE_REMOVE, module->module_name, queue,
                            arena);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
    ret = add_service_tasks(&module_diff->root_services, module->module_name,
                            true, queue);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
    ret = add_service_tasks(&module_diff->user_services, module->module_name,
                            false, queue);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
    if (module_diff->to_link_diff) {
      ret = queue_push(queue, module->to_link ? DOTFILE_LINK : DOTFILE_UNLINK,
                       module->module_name, module->module_name, nullptr, 0,
                       false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    break;
  }
  case REMOVE: {
    const Damgr_Module *module = module_diff->state_module;
    for (size_t i = 0; i < module->root_services_cnt; ++i) {
      ret = queue_push(queue, SERVICE_DISABLE, module->module_name,
                       module->root_services[i], nullptr, 0, true);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < module->user_services_cnt; ++i) {
      ret = queue_push(queue, SERVICE_DISABLE, module->module_name,
                       module->user_services[i], nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->packages_cnt > 0) {
      ret = queue_push(queue, PACKAGE_REMOVE, module->module_name, nullptr,
                       module->packages, module->packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->aur_packages_cnt > 0) {
      ret = queue_push(queue, AUR_PACKAGE_REMOVE, module->module_name, nullptr,
                       module->aur_packages, module->aur_packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->to_link) {
      ret = queue_push(queue, DOTFILE_UNLINK, module->module_name,
                       module->module_name, nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    break;
  }
  }
  return ret;
}

// int damgr_diff_to_queue(Damgr_Host_Diff *host_diff, Damgr_Task_Queue *queue)
// {
int damgr_diff_to_queue(Damgr_Host_Diff *host_diff, Damgr_Task_Queue *queue,
                        Damgr_Arena *arena) {
  int ret = EXIT_SUCCESS;
  // task ordering: remove first, then change, then add
  static const Damgr_Diff_Type order[] = {REMOVE, CHANGE, ADD};
  for (size_t pass = 0; pass < 3 && ret == EXIT_SUCCESS; ++pass) {
    for (size_t i = 0; i < host_diff->diff_cnt && ret == EXIT_SUCCESS; ++i) {
      if (host_diff->diffs[i].diff_type == order[pass])
        ret = add_module_tasks(&host_diff->diffs[i], queue, arena);
    }
  }
  if (ret == EXIT_SUCCESS)
    damgr_log(INFO, "built transaction queue with %zu task(s)", queue->cnt);
  return ret;
}

void damgr_print_queue(Damgr_Task_Queue *queue) {
  damgr_log(INFO, "task queue: %zu task(s)", queue->cnt);
  for (size_t i = 0; i < queue->cnt; ++i) {
    const Damgr_Task *task = &queue->tasks[i];
    char names[DAMGR_PRINT_MAX] = "";
    size_t offset = 0;
    for (size_t j = 0; j < task->packages_cnt && offset < sizeof(names); ++j) {
      offset += (size_t)snprintf(names + offset, sizeof(names) - offset, "%s%s",
                                 j == 0 ? "" : " ", task->packages[j]);
    }
    damgr_log(INFO, "\t%2zu. %-13s module=%s %s %s", i + 1,
              task_type_string(task->type), task->module_name,
              task->name != nullptr ? task->name : "",
              task->packages_cnt > 0 ? names : "");
  }
}
