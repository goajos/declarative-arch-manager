#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/tasks.h"
#include "damgr/utils.h"
#include <stdlib.h>

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
  damgr_get_task_queues_from_configs(&old_config, &config);
  ret = damgr_do_task_queues_for_config(&config);
  // TODO: finish write/remove logic and verify the module queue transaction
  // logic

  return ret;
}
