#include "damgr/task.h"

const char *task_type_string(const Damgr_Task_Type type) {
  switch (type) {
  case PKG_INSTALL:
    return "PKG_INSTALL";
  case PKG_REMOVE:
    return "PKG_REMOVE";
  case AUR_INSTALL:
    return "AUR_INSTALL";
  case AUR_REMOVE:
    return "AUR_REMOVE";
  case SVC_ENABLE:
    return "SVC_ENABLE";
  case SVC_DISABLE:
    return "SVC_DISABLE";
  case LINK:
    return "LINK";
  case UNLINK:
    return "UNLINK";
  }
  return "";
}

const char *task_status_string(const Damgr_Task_Status status) {
  switch (status) {
  case PENDING:
    return "PENDING";
  case SUCCEEDED:
    return "SUCCEEDED";
  case FAILED:
    return "FAILED";
  case ROLLBACK_SUCCEEDED:
    return "ROLLBACK_SUCCEEDED";
  case ROLLBACK_FAILED:
    return "ROLLBACK_FAILED";
  }
  return "PENDING";
}

// queue borrows its memory from the arena
static int push_queue(Damgr_Task_Queue *queue, const Damgr_Task_Type type,
                      const char *module_name, const char *name,
                      char **packages, size_t packages_cnt, bool privileged) {
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

  Damgr_Task task = {.type = type, nullptr}; // zero init for name and packages
  task.privileged = privileged;
  task.status = PENDING;
  task.module_name = module_name != nullptr ? module_name : nullptr;
  task.name = name != nullptr ? name : nullptr;
  task.packages = packages;
  task.packages_cnt = packages_cnt;

  queue->tasks[queue->cnt++] = task;
  return EXIT_SUCCESS;
}

static int push_package_tasks(Damgr_Task_Queue *queue,
                              const Damgr_Array_Diff *array_diff,
                              const Damgr_Task_Type install_type,
                              const Damgr_Task_Type remove_type,
                              const char *module_name, Damgr_Arena *arena) {
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
    char **to_install = damgr_arena_alloc(arena, install_cnt * sizeof(char *));
    if (to_install == nullptr) {
      damgr_log(ERROR, "package task calloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
    size_t j = 0;
    for (size_t i = 0; i < array_diff->active_cnt && ret == EXIT_SUCCESS; ++i)
      if (array_diff->only_in_active[i])
        to_install[j++] = array_diff->active_array[i];
    ret = push_queue(queue, install_type, module_name, nullptr, to_install,
                     install_cnt, false);
  }
  if (ret == EXIT_FAILURE)
    return ret;

  if (remove_cnt > 0) {
    char **to_remove = damgr_arena_alloc(arena, remove_cnt * sizeof(char *));
    if (to_remove == nullptr) {
      damgr_log(ERROR, "package task calloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
    size_t j = 0;
    for (size_t i = 0; i < array_diff->state_cnt && ret == EXIT_SUCCESS; ++i)
      if (array_diff->only_in_state[i])
        to_remove[j++] = array_diff->state_array[i];
    ret = push_queue(queue, remove_type, module_name, nullptr, to_remove,
                     remove_cnt, false);
  }
  return ret;
}

static int push_service_tasks(Damgr_Task_Queue *queue,
                              const Damgr_Array_Diff *array_diff,
                              const char *module_name, bool privileged) {
  int ret = EXIT_SUCCESS;
  for (size_t i = 0; i < array_diff->active_cnt && ret == EXIT_SUCCESS; ++i) {
    if (array_diff->only_in_active[i]) {
      ret = push_queue(queue, SVC_ENABLE, module_name,
                       array_diff->active_array[i], nullptr, 0, privileged);
    }
  }
  if (ret == EXIT_FAILURE)
    return ret;

  for (size_t i = 0; i < array_diff->state_cnt && ret == EXIT_SUCCESS; ++i) {
    if (array_diff->only_in_state[i]) {
      ret = push_queue(queue, SVC_DISABLE, module_name,
                       array_diff->state_array[i], nullptr, 0, privileged);
    }
  }
  return ret;
}

static int push_module_tasks(Damgr_Task_Queue *queue,
                             const Damgr_Module_Diff *module_diff,
                             Damgr_Arena *arena) {
  int ret = EXIT_SUCCESS;
  switch (module_diff->diff_type) {
  case SKIP:
    break;
  case ADD: {
    const Damgr_Module *module = module_diff->active_module;
    if (module->packages_cnt > 0) {
      ret = push_queue(queue, PKG_INSTALL, module->module_name, nullptr,
                       module->packages, module->packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->aur_packages_cnt > 0) {
      ret = push_queue(queue, AUR_INSTALL, module->module_name, nullptr,
                       module->aur_packages, module->aur_packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < module->root_services_cnt; ++i) {
      ret = push_queue(queue, SVC_ENABLE, module->module_name,
                       module->root_services[i], nullptr, 0, true);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < module->user_services_cnt; ++i) {
      ret = push_queue(queue, SVC_ENABLE, module->module_name,
                       module->user_services[i], nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->to_link) {
      ret = push_queue(queue, LINK, module->module_name, module->module_name,
                       nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    break;
  }
  case CHANGE: {
    const Damgr_Module *module = module_diff->active_module;
    ret = push_package_tasks(queue, &module_diff->packages, PKG_INSTALL,
                             PKG_REMOVE, module->module_name, arena);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
    ret = push_package_tasks(queue, &module_diff->aur_packages, AUR_INSTALL,
                             AUR_REMOVE, module->module_name, arena);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
    ret = push_service_tasks(queue, &module_diff->root_services,
                             module->module_name, true);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
    ret = push_service_tasks(queue, &module_diff->user_services,
                             module->module_name, false);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
    if (module_diff->to_link_diff) {
      ret = push_queue(queue, module->to_link ? LINK : UNLINK,
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
      ret = push_queue(queue, SVC_DISABLE, module->module_name,
                       module->root_services[i], nullptr, 0, true);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < module->user_services_cnt; ++i) {
      ret = push_queue(queue, SVC_DISABLE, module->module_name,
                       module->user_services[i], nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->packages_cnt > 0) {
      ret = push_queue(queue, PKG_REMOVE, module->module_name, nullptr,
                       module->packages, module->packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->aur_packages_cnt > 0) {
      ret = push_queue(queue, AUR_REMOVE, module->module_name, nullptr,
                       module->aur_packages, module->aur_packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->to_link) {
      ret = push_queue(queue, UNLINK, module->module_name, module->module_name,
                       nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    break;
  }
  }
  return ret;
}

int damgr_fill_queue(Damgr_Task_Queue *queue, const Damgr_Host_Diff *host_diff,
                     Damgr_Arena *arena) {
  int ret = EXIT_SUCCESS;
  // task ordering: remove first, then change, then add
  for (size_t i = 0; i < host_diff->diff_cnt && ret == EXIT_SUCCESS; ++i) {
    if (host_diff->diffs[i].diff_type == REMOVE)
      ret = push_module_tasks(queue, &host_diff->diffs[i], arena);
  }
  if (ret == EXIT_FAILURE)
    return ret;

  for (size_t i = 0; i < host_diff->diff_cnt && ret == EXIT_SUCCESS; ++i) {
    if (host_diff->diffs[i].diff_type == CHANGE)
      ret = push_module_tasks(queue, &host_diff->diffs[i], arena);
  }
  if (ret == EXIT_FAILURE)
    return ret;

  for (size_t i = 0; i < host_diff->diff_cnt && ret == EXIT_SUCCESS; ++i) {
    if (host_diff->diffs[i].diff_type == ADD)
      ret = push_module_tasks(queue, &host_diff->diffs[i], arena);
  }
  if (ret == EXIT_FAILURE)
    return ret;

  if (ret == EXIT_SUCCESS)
    damgr_log(INFO, "built transaction queue with %zu task(s)", queue->cnt);
  return ret;
}

void damgr_free_queue(Damgr_Task_Queue *queue) {
  free(queue->tasks);
  queue->tasks = nullptr;
  queue->cnt = 0;
  queue->capacity = 0;
}

int do_task(const Damgr_Paths *paths, const char *aur_helper,
            const Damgr_Task *task) {
  switch (task->type) {
  case PKG_INSTALL:
    return run_package_command(true, task->packages, task->packages_cnt);
  case AUR_INSTALL:
    return run_aur_install_command(aur_helper, task->packages,
                                   task->packages_cnt);

  case PKG_REMOVE:
  case AUR_REMOVE:
    return run_package_command(false, task->packages, task->packages_cnt);

  case SVC_ENABLE:
  case SVC_DISABLE:
    return run_service_command(task->privileged, task->type == SVC_ENABLE,
                               task->name);
  case LINK:
    return do_dotfile_link(paths, task->name);

  case UNLINK:
    return do_dotfile_unlink(paths, task->name);
  }

  return EXIT_FAILURE;
}

int undo_task(const Damgr_Paths *paths, const char *aur_helper,
              const Damgr_Task *task) {
  switch (task->type) {
  case PKG_INSTALL:
  case AUR_INSTALL:
    return run_package_command(false, task->packages, task->packages_cnt);

  case PKG_REMOVE:
    return run_package_command(true, task->packages, task->packages_cnt);
  case AUR_REMOVE:
    return run_aur_install_command(aur_helper, task->packages,
                                   task->packages_cnt);

  case SVC_ENABLE:
    return run_service_command(task->privileged, false, task->name);
  case SVC_DISABLE:
    return run_service_command(task->privileged, true, task->name);

  case LINK:
    return do_dotfile_unlink(paths, task->name);
  case UNLINK:
    return do_dotfile_link(paths, task->name);
  }

  return EXIT_FAILURE;
}
