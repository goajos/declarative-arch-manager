#include "damgr/commit.h"
#include "damgr/diff.h"
#include "damgr/log.h"
#include "damgr/stage.h"
#include "damgr/state.h"
#include "damgr/transaction.h"

// TODO: warning if .staging and/or .backup folder exists?
int damgr_merge() {
  Damgr_Arena *arena = calloc(1, sizeof(Damgr_Arena));
  if (arena == nullptr)
    return EXIT_FAILURE;
  damgr_arena_init(arena);

  Damgr_Config config = {0};

  int ret;
  char fidbuf[DAMGR_PATH_MAX];
  if (getenv("HOME") == nullptr) {
    damgr_log(ERROR, "getenv(\"HOME\") return a nullptr");
    ret = EXIT_FAILURE;
    goto cleanup;
  }
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr", getenv("HOME"));
  ret = damgr_count_state_dir(fidbuf);
  if (ret == EXIT_FAILURE) {
    // no dir or unable to read dir
    goto cleanup;
  } else if (ret == -1) { // no .conf files found
    damgr_log(INFO, "no state to parse, the state directory is empty: %s",
              fidbuf);
  } else {
    ret = damgr_read_state(&config, arena);
    if (ret == EXIT_FAILURE)
      goto cleanup;
  }
  ret = damgr_read_active(&config, arena);
  if (ret == EXIT_FAILURE)
    goto cleanup;

  Damgr_Host_Diff host_diff = {0};
  ret = damgr_get_diff_host(&host_diff, config.state_host, config.active_host,
                            arena);
  if (ret == EXIT_FAILURE)
    goto cleanup;

  // TODO: check journal?

  Damgr_Task_Queue queue = {0};
  ret = damgr_diff_to_queue(&host_diff, &queue, arena);
  if (ret == EXIT_FAILURE)
    goto free_queue;

  if (queue.cnt == 0) {
    damgr_log(INFO, "no changes to apply, state is up to date");
    ret = EXIT_SUCCESS;
    goto free_queue;
  }
  damgr_print_queue(&queue);

  ret = damgr_preflight_check(&config, &queue);
  if (ret == EXIT_FAILURE)
    goto free_queue;

  // TODO: write journal?

  ret = damgr_do_transaction(&config, &queue);
  if (ret == EXIT_FAILURE)
    goto free_queue;

  ret = damgr_stage_with_config(&config);
  if (ret == EXIT_FAILURE) {
    damgr_log(ERROR,
              "failed to stage active config:\ndo NOT rerun damgr merge or"
              "change .conf files, manually run damgr stage and damgr commit!");
    goto free_queue;
  }

  if (config.state_host != nullptr) {
    ret = damgr_commit_with_config(&config);
    if (ret == EXIT_FAILURE) {
      damgr_log(ERROR, "failed to commit:\ndo NOT rerun damgr merge or"
                       "change .conf files, manually run damgr commit!");
      goto free_queue;
    }
  }

  ret = EXIT_SUCCESS;
free_queue:
  damgr_free_queue(&queue);
cleanup:
  free(arena);
  return ret;
}
