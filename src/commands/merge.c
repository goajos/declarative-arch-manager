#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/tasks.h"
#include "damgr/utils.h"
#include <stdlib.h>
#include <string.h>

int damgr_merge() {
  damgr_log(INFO, "running damgr merge...");

  int ret;
  char fidbuf[damgr_path_max];
  Damgr_Config old_config = {};
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr", getenv("HOME"));
  ret = damgr_is_state_dir_empty(fidbuf);
  if (ret == EXIT_FAILURE) {
    damgr_log(ERROR, "failed to open state directory: %s", fidbuf);
    return EXIT_FAILURE;
  } else if (ret == 2) { // only "." and ".." dirs found
    damgr_log(INFO, "no state to parse, the state directory is empty: %s",
              fidbuf);
  } else {
    damgr_read_config(&old_config, true);
    damgr_read_host(&old_config, true);
    for (size_t i = 0; i < old_config.active_host->modules.count; ++i) {
      damgr_read_module(&old_config, i, true);
    }
  }

  Damgr_Config config = {};
  damgr_read_config(&config, false);
  damgr_read_host(&config, false);
  for (size_t i = 0; i < config.active_host->modules.count; ++i) {
    damgr_read_module(&config, i, false);
  }

  // DEBUG:
  // *(Damgr_Task_Queue *)config->active_host.task_queues.ptrs[0]
  // *(Damgr_Task *)(*(Damgr_Task_Queue *)config->
  //    active_host.task_queues.ptrs[0]).tasks.ptrs[1]
  if (old_config.active_host != nullptr) {
    ret = strcmp(old_config.active_host->host_name,
                 config.active_host->host_name);
    if (ret < 0 || ret > 0) { // different host
      damgr_get_module_task_queues_from_host(config.active_host);
      damgr_get_module_task_queues_from_host(old_config.active_host);
    } else { // same host
      damgr_get_module_task_queues_from_hosts_diff(old_config.active_host,
                                                   config.active_host);
    }

    for (size_t i = 0; i < config.active_host->modules.count; ++i) {
      Damgr_Module *module = config.active_host->modules.ptrs[i];
      if (module->path != nullptr) {
        ret = damgr_do_module_task_queue(module, config.aur_helper);
        if (ret != EXIT_SUCCESS) {
          damgr_undo_module_task_queue(module);
          module->path = nullptr;
          damgr_log(ERROR,
                    "manual fixing might be required for new module: %s!",
                    module->module_name);
          for (size_t j = 0; j < module->task_queue.count; ++j) {
            Damgr_Task *task = (Damgr_Task *)module->task_queue.ptrs[j];
            if (task->status == FAILED) {
              damgr_log(ERROR, "\tfailed task: %s!",
                        task->payload.payload_name);
            } else if (task->status == PENDING) {
              damgr_log(ERROR, "\tpending task: %s!",
                        task->payload.payload_name);
            }
          }
        }
      }
    }

    for (size_t i = 0; i < old_config.active_host->modules.count; ++i) {
      Damgr_Module *old_module = old_config.active_host->modules.ptrs[i];
      if (old_module->state_path != nullptr) {
        ret = damgr_do_module_task_queue(old_module, config.aur_helper);
        if (ret != EXIT_SUCCESS) {
          old_module->state_path = nullptr;
          damgr_log(ERROR,
                    "manual cleanup might be required for old module: %s!",
                    old_module->module_name);
          for (size_t j = 0; j < old_module->task_queue.count; ++j) {
            Damgr_Task *task = (Damgr_Task *)old_module->task_queue.ptrs[j];
            if (task->status == FAILED) {
              damgr_log(ERROR, "\tfailed task: %s!",
                        task->payload.payload_name);
            } else if (task->status == PENDING) {
              damgr_log(ERROR, "\tpending task: %s!",
                        task->payload.payload_name);
            }
          }
        }
      }
    }
  } else { // no old config
    damgr_get_module_task_queues_from_host(config.active_host);
    for (size_t i = 0; i < config.active_host->modules.count; ++i) {
      Damgr_Module *module = config.active_host->modules.ptrs[i];
      if (module->path != nullptr) {
        ret = damgr_do_module_task_queue(module, config.aur_helper);
        if (ret != EXIT_SUCCESS) {
          damgr_undo_module_task_queue(module);
          module->path = nullptr;
          damgr_log(ERROR,
                    "manual cleanup might be required for new module: %s!",
                    module->module_name);
          for (size_t j = 0; j < module->task_queue.count; ++j) {
            Damgr_Task *task = (Damgr_Task *)module->task_queue.ptrs[j];
            if (task->status == FAILED) {
              damgr_log(ERROR, "\tfailed task: %s!",
                        task->payload.payload_name);
            } else if (task->status == PENDING) {
              damgr_log(ERROR, "\tpending task: %s!",
                        task->payload.payload_name);
            }
          }
        }
      }
    }
  }

  // damgr_free_module(old_module);
  // old_host->modules.ptrs[j] = nullptr;

  // damgr_get_task_queues(&old_config, &config);
  // ret = damgr_do_task_queues_for_config(&config);
  // TODO: finish write/remove logic and verify the module queue transaction
  // logic

  return ret;
}
