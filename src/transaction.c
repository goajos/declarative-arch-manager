#include "damgr/transaction.h"

static bool unit_file_exists_in(const char *dir, const char *unit) {
  char fid_buf[DAMGR_FID_BUF_MAX];
  // exact match
  snprintf(fid_buf, sizeof(fid_buf), "%s/%s", dir, unit);
  if (access(fid_buf, F_OK) == 0)
    return true;
  // append .service
  snprintf(fid_buf, sizeof(fid_buf), "%s/%s.service", dir, unit);
  return access(fid_buf, F_OK) == 0;
}

static bool unit_file_exists(const char *unit, const bool privileged) {
  if (privileged) {
    static const char *const dirs[] = {"/etc/systemd/system",
                                       "/usr/local/lib/systemd/system",
                                       "/usr/lib/systemd/system"};
    for (size_t i = 0; i < 3; ++i)
      if (unit_file_exists_in(dirs[i], unit))
        return true;
  } else {
    char config_user_dir[DAMGR_FID_BUF_MAX];
    char local_user_dir[DAMGR_FID_BUF_MAX];
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

// TODO: check if services that should be enabled are enabled?
int damgr_transaction_preflight(const Damgr_Context *context,
                                const Damgr_Task_Queue *queue) {
  damgr_log(INFO, "running preflight checks");
  char fid_buf[DAMGR_FID_BUF_MAX];
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
      snprintf(fid_buf, sizeof(fid_buf), "/usr/bin/%s", context->aur_helper);
      if (access(fid_buf, X_OK) != 0) {
        damgr_log(ERROR, "preflight failed: aur helper is not executable: %s",
                  fid_buf);
        return EXIT_FAILURE;
      }
      break;
    case LINK:
      snprintf(fid_buf, sizeof(fid_buf), "%s/.config/damgr/dotfiles/%s",
               getenv("HOME"), task->name);
      if (access(fid_buf, F_OK) != 0) {
        damgr_log(ERROR, "preflight failed: dotfile source is missing: %s",
                  fid_buf);
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

static void transaction_rollback(const char *aur_helper,
                                 const size_t succeeded_cnt,
                                 Damgr_Task_Queue *queue) {
  damgr_log(INFO, "rolling back %zu succeeded task(s) in reverse order",
            succeeded_cnt);
  for (size_t i = succeeded_cnt; i-- > 0;) {
    Damgr_Task *task = &queue->tasks[i];
    if (undo_task(aur_helper, task) == EXIT_SUCCESS) {
      task->status = ROLLBACK_SUCCEEDED;
      damgr_log(INFO, "successful rollback %s module=%s",
                task_type_string(task->type), task->module_name);
    } else {
      task->status = ROLLBACK_FAILED;
      damgr_log(ERROR,
                "failed to rollback %s module=%s - manual cleanup required!",
                task_type_string(task->type), task->module_name);
    }
  }
}

int damgr_transaction(const char *aur_helper, Damgr_Task_Queue *queue) {
  damgr_log(INFO, "starting transaction: %zu task(s)", queue->cnt);
  size_t succeeded_cnt = 0;
  for (; succeeded_cnt < queue->cnt; ++succeeded_cnt) {
    Damgr_Task *task = &queue->tasks[succeeded_cnt];
    damgr_log(INFO, "running [%zu/%zu] %s module=%s", succeeded_cnt + 1,
              queue->cnt, task_type_string(task->type), task->module_name);
    if (do_task(aur_helper, task) == EXIT_SUCCESS) {
      task->status = SUCCEEDED;
    } else {
      task->status = FAILED;
      damgr_log(ERROR, "transaction failed at task %zu: %s module=%s",
                succeeded_cnt + 1, task_type_string(task->type),
                task->module_name);
      transaction_rollback(aur_helper, succeeded_cnt, queue);
      // TODO: report residual (failed and pending tasks after rollback?)
      return EXIT_FAILURE;
    }
  }
  damgr_log(INFO, "transaction succeeded: %zu task(s) done", queue->cnt);
  return EXIT_SUCCESS;
}
