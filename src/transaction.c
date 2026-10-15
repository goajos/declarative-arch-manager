#define _POSIX_C_SOURCE 200809L
#include "damgr/transaction.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *task_type_string(Damgr_Task_Type type) {
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

void damgr_free_queue(Damgr_Task_Queue *queue) {
  free(queue->tasks);
  queue->tasks = nullptr;
  queue->cnt = 0;
  queue->capacity = 0;
}

// queue borrows its memory from the arena
static int queue_push(Damgr_Task_Queue *queue, Damgr_Task_Type type,
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
    char **to_install = damgr_arena_alloc(arena, install_cnt * sizeof(char *));
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
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
  }
  if (remove_cnt > 0) {
    char **to_remove = damgr_arena_alloc(arena, remove_cnt * sizeof(char *));
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
      ret = queue_push(queue, SVC_ENABLE, module_name,
                       array_diff->active_array[i], nullptr, 0, privileged);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
  }
  for (size_t i = 0; i < array_diff->state_cnt; ++i) {
    if (array_diff->only_in_state[i]) {
      ret = queue_push(queue, SVC_DISABLE, module_name,
                       array_diff->state_array[i], nullptr, 0, privileged);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
  }
  return ret;
}

static int add_module_tasks(Damgr_Module_Diff *module_diff,
                            Damgr_Task_Queue *queue, Damgr_Arena *arena) {
  int ret = EXIT_SUCCESS;
  switch (module_diff->diff_type) {
  case SKIP:
    break;
  case ADD: {
    const Damgr_Module *module = module_diff->active_module;
    if (module->packages_cnt > 0) {
      ret = queue_push(queue, PKG_INSTALL, module->module_name, nullptr,
                       module->packages, module->packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->aur_packages_cnt > 0) {
      ret = queue_push(queue, AUR_INSTALL, module->module_name, nullptr,
                       module->aur_packages, module->aur_packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < module->root_services_cnt; ++i) {
      ret = queue_push(queue, SVC_ENABLE, module->module_name,
                       module->root_services[i], nullptr, 0, true);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < module->user_services_cnt; ++i) {
      ret = queue_push(queue, SVC_ENABLE, module->module_name,
                       module->user_services[i], nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->to_link) {
      ret = queue_push(queue, LINK, module->module_name, module->module_name,
                       nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    break;
  }
  case CHANGE: {
    const Damgr_Module *module = module_diff->active_module;
    ret = add_package_tasks(&module_diff->packages, PKG_INSTALL, PKG_REMOVE,
                            module->module_name, queue, arena);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
    ret = add_package_tasks(&module_diff->aur_packages, AUR_INSTALL, AUR_REMOVE,
                            module->module_name, queue, arena);
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
      ret = queue_push(queue, module->to_link ? LINK : UNLINK,
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
      ret = queue_push(queue, SVC_DISABLE, module->module_name,
                       module->root_services[i], nullptr, 0, true);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < module->user_services_cnt; ++i) {
      ret = queue_push(queue, SVC_DISABLE, module->module_name,
                       module->user_services[i], nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->packages_cnt > 0) {
      ret = queue_push(queue, PKG_REMOVE, module->module_name, nullptr,
                       module->packages, module->packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->aur_packages_cnt > 0) {
      ret = queue_push(queue, AUR_REMOVE, module->module_name, nullptr,
                       module->aur_packages, module->aur_packages_cnt, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    if (module->to_link) {
      ret = queue_push(queue, UNLINK, module->module_name, module->module_name,
                       nullptr, 0, false);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
    break;
  }
  }
  return ret;
}

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

static bool unit_file_exists_in(const char *dir, const char *unit) {
  char path[DAMGR_PATH_MAX];
  // exact match
  snprintf(path, sizeof(path), "%s/%s", dir, unit);
  if (access(path, F_OK) == 0)
    return true;
  // append .service
  snprintf(path, sizeof(path), "%s/%s.service", dir, unit);
  return access(path, F_OK) == 0;
}

static bool unit_file_exists(const char *unit, bool privileged) {
  if (privileged) {
    static const char *const dirs[] = {"/etc/systemd/system",
                                       "/usr/local/lib/systemd/system",
                                       "/usr/lib/systemd/system"};
    for (size_t i = 0; i < 3; ++i)
      if (unit_file_exists_in(dirs[i], unit))
        return true;
  } else {
    char config_user_dir[DAMGR_PATH_MAX];
    char local_user_dir[DAMGR_PATH_MAX];
    snprintf(config_user_dir, sizeof(config_user_dir),
             "%s/.config/systemd/user", getenv("HOME"));
    snprintf(local_user_dir, sizeof(local_user_dir),
             "%s/.local/share/systemd/user", getenv("HOME"));
    return unit_file_exists_in(config_user_dir, unit) ||
           unit_file_exists_in(local_user_dir, unit) ||
           unit_file_exists_in("/etc/systemd/user", unit) ||
           unit_file_exists_in("/usr/local/lib/systemd/user", unit) ||
           unit_file_exists_in("/usr/lib/systemd/user", unit);
  }
  return false;
}

int damgr_preflight_check(Damgr_Config *config, Damgr_Task_Queue *queue) {
  damgr_log(INFO, "running preflight checks");
  char path[DAMGR_PATH_MAX];
  for (size_t i = 0; i < queue->cnt; ++i) {
    const Damgr_Task *task = &queue->tasks[i];
    switch (task->type) {
    case PKG_INSTALL:
    case PKG_REMOVE:
    case AUR_REMOVE:
      if (access("/usr/bin/sudo", X_OK) != 0) {
        damgr_log(ERROR, "preflight failed: /usr/bin/sudo is not executable");
        return EXIT_FAILURE;
      }
      break;
    case AUR_INSTALL:
      snprintf(path, sizeof(path), "/usr/bin/%s", config->aur_helper);
      if (access(path, X_OK) != 0) {
        damgr_log(ERROR, "preflight failed: aur helper is not executable: %s",
                  path);
        return EXIT_FAILURE;
      }
      break;
    case LINK:
      snprintf(path, sizeof(path), "%s/.config/damgr/dotfiles/%s",
               getenv("HOME"), task->name);
      if (access(path, F_OK) != 0) {
        damgr_log(ERROR, "preflight failed: dotfile source is missing: %s",
                  path);
        return EXIT_FAILURE;
      }
      break;
    case SVC_ENABLE:
    case SVC_DISABLE:
      if (task->privileged && access("/usr/bin/sudo", X_OK) != 0) {
        damgr_log(ERROR, "preflight failed: /usr/bin/sudo is not executable");
        return EXIT_FAILURE;
      }
      if (!unit_file_exists(task->name, task->privileged)) {
        damgr_log(ERROR, "systemd unit file not found for: %s", task->name);
        return EXIT_FAILURE;
      }
      break;
    case UNLINK:
      break;
    }
  }
  damgr_log(INFO, "preflight checks passed");
  return EXIT_SUCCESS;
}

static int execute_execv(const char *path, char *const argv[]) {
  pid_t pid = fork();
  if (pid == -1) {
    damgr_log(ERROR, "fork failed: %s", strerror(errno));
    return EXIT_FAILURE;
  } else if (pid == 0) {
    execv(path, argv);
    // only reached if execv failed
    _exit(EXIT_FAILURE);
  } else {
    int wstatus;
    if (waitpid(pid, &wstatus, 0) != pid) {
      damgr_log(ERROR, "waitpid failed: %s", strerror(errno));
      return EXIT_FAILURE;
    }
    if (WIFEXITED(wstatus)) {
      if (WEXITSTATUS(wstatus) != EXIT_SUCCESS) {
        damgr_log(ERROR, "command %s failed with exit code: %d", path,
                  WEXITSTATUS(wstatus));
        return EXIT_FAILURE;
      }
    } else if (WIFSIGNALED(wstatus)) {
      damgr_log(ERROR, "command %s was terminated by signal: %d", path,
                WTERMSIG(wstatus));
      return EXIT_FAILURE;
    } else {
      damgr_log(ERROR, "command %s ended in unexpected state: 0x%x", path,
                wstatus);
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

static int run_package_command(bool install, char *const *names, size_t cnt) {
  char **argv = malloc((cnt + (install ? 5 : 4)) * sizeof(char *));
  if (argv == nullptr) {
    damgr_log(ERROR, "malloc failed for package %s command argv",
              install ? "install" : "remove");
    return EXIT_FAILURE;
  }
  argv[0] = "sudo";
  argv[1] = "pacman";
  if (install) {
    argv[2] = "-S";
    argv[3] = "--needed";
    for (size_t i = 0; i < cnt; ++i)
      argv[4 + i] = names[i];
    argv[4 + cnt] = nullptr;
  } else {
    // argv[2] = "-Rns"; // also removes dependencies?
    argv[2] = "-Rn";
    for (size_t i = 0; i < cnt; ++i)
      argv[3 + i] = names[i];
    argv[3 + cnt] = nullptr;
  }
  int ret = execute_execv("/usr/bin/sudo", argv);
  free(argv);
  return ret;
}

static int run_aur_install_command(const char *helper, char *const *names,
                                   size_t cnt) {
  char path[DAMGR_PATH_MAX];
  snprintf(path, sizeof(path), "/usr/bin/%s", helper);
  char **argv = malloc((cnt + 4) * sizeof(char *));
  if (argv == nullptr) {
    damgr_log(ERROR, "malloc failed for aur install command argv");
    return EXIT_FAILURE;
  }
  argv[0] = (char *)helper;
  argv[1] = "-S";
  argv[2] = "--needed";
  for (size_t i = 0; i < cnt; ++i)
    argv[3 + i] = names[i];
  argv[3 + cnt] = nullptr;
  int ret = execute_execv(path, argv);
  free(argv);
  return ret;
}

static int run_service_command(bool privileged, bool to_enable,
                               const char *service) {
  char **argv = malloc(6 * sizeof(char *));
  if (argv == nullptr) {
    damgr_log(ERROR, "malloc failed for %s %s service command argv",
              to_enable ? "enable" : "disable", privileged ? "root" : "user");
    return EXIT_FAILURE;
  }
  if (privileged) {
    argv[0] = "sudo";
    argv[1] = "systemctl";
    argv[2] = "--now";
    argv[3] = to_enable ? "enable" : "disable";
    argv[4] = (char *)service;
    argv[5] = nullptr;
  } else {
    argv[0] = "systemctl";
    argv[1] = "--user";
    argv[2] = "--now";
    argv[3] = to_enable ? "enable" : "disable";
    argv[4] = (char *)service;
    argv[5] = nullptr;
  }
  int ret =
      execute_execv(privileged ? "/usr/bin/sudo" : "/usr/bin/systemctl", argv);
  free(argv);
  return ret;
}

static int do_dotfile_link(const char *dotfile) {
  char src[DAMGR_PATH_MAX];
  char dst[DAMGR_PATH_MAX];
  snprintf(src, sizeof(src), "%s/.config/damgr/dotfiles/%s", getenv("HOME"),
           dotfile);
  snprintf(dst, sizeof(dst), "%s/.config/%s", getenv("HOME"), dotfile);

  struct stat st;
  if (lstat(dst, &st) == 0) {
    if (S_ISLNK(st.st_mode)) {
      return EXIT_SUCCESS; // already linked, idempotent
    }
    damgr_log(ERROR,
              "dotfile destination exists and is not a symlink, refusing to "
              "overwrite: %s",
              dst);
    return EXIT_FAILURE;
  }
  if (symlink(src, dst) != 0) {
    damgr_log(ERROR, "could not symlink dotfile %s -> %s: %s", src, dst,
              strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

static int do_dotfile_unlink(const char *dotfile) {
  char dst[DAMGR_PATH_MAX];
  snprintf(dst, sizeof(dst), "%s/.config/%s", getenv("HOME"), dotfile);
  if (unlink(dst) != 0 && errno != ENOENT) {
    damgr_log(ERROR, "could not unlink dotfile %s: %s", dst, strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

static int do_task(Damgr_Config *config, Damgr_Task *task) {
  switch (task->type) {
  case PKG_INSTALL:
    return run_package_command(true, task->packages, task->packages_cnt);
  case AUR_INSTALL:
    return run_aur_install_command(config->aur_helper, task->packages,
                                   task->packages_cnt);

  case PKG_REMOVE:
  case AUR_REMOVE:
    return run_package_command(false, task->packages, task->packages_cnt);

  case SVC_ENABLE:
  case SVC_DISABLE:
    return run_service_command(task->privileged, task->type == SVC_ENABLE,
                               task->name);
  case LINK:
    return do_dotfile_link(task->name);

  case UNLINK:
    return do_dotfile_unlink(task->name);
  }

  return EXIT_FAILURE;
}

static int undo_task(Damgr_Config *config, Damgr_Task *task) {
  switch (task->type) {
  case PKG_INSTALL:
  case AUR_INSTALL:
    return run_package_command(false, task->packages, task->packages_cnt);

  case PKG_REMOVE:
    return run_package_command(true, task->packages, task->packages_cnt);
  case AUR_REMOVE:
    return run_aur_install_command(config->aur_helper, task->packages,
                                   task->packages_cnt);

  case SVC_ENABLE:
    return run_service_command(task->privileged, false, task->name);
  case SVC_DISABLE:
    return run_service_command(task->privileged, true, task->name);

  case LINK:
    return do_dotfile_unlink(task->name);
  case UNLINK:
    return do_dotfile_link(task->name);
  }

  return EXIT_FAILURE;
}

static void rollback(Damgr_Config *config, Damgr_Task_Queue *queue,
                     size_t succeeded_cnt) {
  damgr_log(INFO, "rolling back %zu succeeded task(s) in reverse order",
            succeeded_cnt);
  for (size_t i = succeeded_cnt; i-- > 0;) {
    Damgr_Task *task = &queue->tasks[i];
    if (task->status != SUCCEEDED)
      continue;
    if (undo_task(config, task) == EXIT_SUCCESS) {
      task->status = ROLLBACK;
      damgr_log(INFO, "rolled back %s module=%s", task_type_string(task->type),
                task->module_name);
    } else {
      task->status = FAILED;
      damgr_log(
          ERROR,
          "failed to roll back %s module=%s - manual fixing might be required",
          task_type_string(task->type), task->module_name);
    }
  }
}

int damgr_do_transaction(Damgr_Config *config, Damgr_Task_Queue *queue) {
  damgr_log(INFO, "starting transaction: %zu task(s)", queue->cnt);
  size_t succeeded_cnt = 0;
  for (; succeeded_cnt < queue->cnt; ++succeeded_cnt) {
    Damgr_Task *task = &queue->tasks[succeeded_cnt];
    damgr_log(INFO, "running [%zu/%zu] %s module=%s", succeeded_cnt + 1,
              queue->cnt, task_type_string(task->type), task->module_name);
    if (do_task(config, task) == EXIT_SUCCESS) {
      task->status = SUCCEEDED;
    } else {
      task->status = FAILED;
      damgr_log(ERROR, "transaction failed at task %zu: %s module=%s",
                succeeded_cnt + 1, task_type_string(task->type),
                task->module_name);
      rollback(config, queue, succeeded_cnt);
      // TODO: report residual (failed and pending tasks after rollback?)
      return EXIT_FAILURE;
    }
  }
  damgr_log(INFO, "transaction succeeded: %zu task(s) done", queue->cnt);
  return EXIT_SUCCESS;
}
