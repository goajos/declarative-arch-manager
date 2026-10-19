#include "damgr/merge.h"

int damgr_merge(const Damgr_Paths *paths, Damgr_Arena *arena) {

  int ret;
  // state folder should be created before merge
  if (!damgr_make_dir(paths->state_dir)) {
    return EXIT_FAILURE;
  }

  int phase = -1;
  int ntasks = -1;
  if (damgr_read_journal(paths->journal, &phase, &ntasks) == EXIT_SUCCESS) {
    if (phase == DAMGR_JOURNAL_DONE) {
      if (damgr_dir_exists(paths->staging_dir)) {
        damgr_log(ERROR,
                  "a previous transaction finished but the commit did not, "
                  "the .staging folder is still present, re-run damgr commit "
                  "to retry the commit");
        return EXIT_FAILURE;
      }
      // stale DONE journal
      if (damgr_clean_journal(paths->journal) != EXIT_SUCCESS)
        return EXIT_FAILURE;
    } else {
      damgr_log(ERROR,
                "found a journal with phase %s from a previous interrupted "
                "transaction, refusing to merge, inspect the journal, clean "
                "up the residual tasks manually and remove it first: %s",
                journal_phase_string((Damgr_Journal_Phase)phase),
                paths->journal);
      return EXIT_FAILURE;
    }
  }

  // staging folder shouldn't exist before merge
  if (damgr_dir_exists(paths->staging_dir)) {
    damgr_log(ERROR, "trying to merge with a .staging folder");
    return EXIT_FAILURE;
  }

  // backup folder shouldn't exist before merge
  if (damgr_dir_exists(paths->backup_dir)) {
    damgr_log(ERROR, "trying to merge with a backup folder");
    return EXIT_FAILURE;
  }

  Damgr_Context context = {0};
  if (damgr_is_dir_empty(paths->state_dir)) {
    damgr_log(INFO, "no state to parse, the state directory is empty: %s",
              paths->state_dir);
  } else {
    ret = damgr_read_state_context(paths, &context, arena);
    if (ret == EXIT_FAILURE)
      return ret;
  }
  ret = damgr_read_active_context(paths, &context, arena);
  if (ret == EXIT_FAILURE)
    return ret;

  Damgr_Host_Diff host_diff = {0};
  ret = damgr_diff_host(&host_diff, context.state_host, context.active_host,
                        arena);
  if (ret == EXIT_FAILURE)
    return ret;

  Damgr_Task_Queue queue = {0};
  ret = damgr_fill_queue(&queue, &host_diff, arena);
  if (ret == EXIT_FAILURE) {
    damgr_free_queue(&queue);
    return ret;
  }

  if (queue.cnt == 0) {
    damgr_log(INFO, "no changes to apply, state is up to date");
    damgr_free_queue(&queue);
    return ret;
  }

  ret = damgr_transaction_preflight(paths, &context, &queue);
  if (ret == EXIT_FAILURE) {
    damgr_free_queue(&queue);
    return ret;
  }

  ret = damgr_transaction(paths, context.aur_helper, &queue);
  if (ret == EXIT_FAILURE) {
    damgr_free_queue(&queue);
    return ret;
  }

  ret = damgr_stage_with_context(&context, paths);
  if (ret == EXIT_FAILURE) {
    damgr_free_queue(&queue);
    return ret;
  }

  ret = damgr_commit(paths);
  if (ret == EXIT_FAILURE) {
    damgr_free_queue(&queue);
    return ret;
  }

  damgr_free_queue(&queue);
  damgr_log(INFO, "finished damgr merge");
  return EXIT_SUCCESS;
}
