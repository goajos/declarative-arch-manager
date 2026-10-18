#include "damgr/merge.h"

int damgr_merge(Damgr_Arena *arena) {

  int ret;
  char state_buf[DAMGR_FID_BUF_MAX];
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  // state folder should be created before merge
  if (!damgr_make_dir(state_buf)) {
    return EXIT_FAILURE;
  }

  char staging_buf[DAMGR_FID_BUF_MAX];
  snprintf(staging_buf, sizeof(staging_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STAGING));

  char journal_buf[DAMGR_FID_BUF_MAX + sizeof(DAMGR_JOURNAL)];
  snprintf(journal_buf, sizeof(journal_buf), "%s/%s", state_buf, DAMGR_JOURNAL);
  int phase = -1;
  int ntasks = -1;
  if (damgr_read_journal(journal_buf, &phase, &ntasks) == EXIT_SUCCESS) {
    if (phase == DAMGR_JOURNAL_DONE) {
      if (damgr_dir_exists(staging_buf)) {
        damgr_log(ERROR,
                  "a previous transaction finished but the commit did not, "
                  "the .staging folder is still present, re-run damgr commit "
                  "to retry the commit");
        return EXIT_FAILURE;
      }
      // stakle DONE journal
      if (damgr_clean_journal(journal_buf) != EXIT_SUCCESS)
        return EXIT_FAILURE;
    } else {
      damgr_log(ERROR,
                "found a journal with phase %s from a previous interrupted "
                "transaction, refusing to merge, inspect the journal, clean "
                "up the residual tasks manually and remove it first: %s",
                journal_phase_string((Damgr_Journal_Phase)phase), journal_buf);
      return EXIT_FAILURE;
    }
  }

  // staging folder shouldn't exist before merge
  if (damgr_dir_exists(staging_buf)) {
    damgr_log(ERROR, "trying to merge with a .staging folder");
    return EXIT_FAILURE;
  }

  char backup_buf[DAMGR_FID_BUF_MAX];
  snprintf(backup_buf, sizeof(backup_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(BACKUP));
  // backup folder shouldn't exist before merge
  if (damgr_dir_exists(backup_buf)) {
    damgr_log(ERROR, "trying to merge with a backup folder");
    return EXIT_FAILURE;
  }

  Damgr_Context context = {nullptr};
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  if (damgr_is_dir_empty(state_buf)) {
    damgr_log(INFO, "no state to parse, the state directory is empty: %s",
              state_buf);
  } else {
    ret = damgr_read_state_context(&context, arena);
    if (ret == EXIT_FAILURE)
      return ret;
  }
  ret = damgr_read_active_context(&context, arena);
  if (ret == EXIT_FAILURE)
    return ret;

  Damgr_Host_Diff host_diff = {nullptr};
  ret = damgr_diff_host(&host_diff, context.state_host, context.active_host,
                        arena);
  if (ret == EXIT_FAILURE)
    return ret;

  Damgr_Task_Queue queue = {nullptr};
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

  ret = damgr_transaction_preflight(&context, &queue);
  if (ret == EXIT_FAILURE) {
    damgr_free_queue(&queue);
    return ret;
  }

  ret = damgr_transaction(context.aur_helper, journal_buf, &queue);
  if (ret == EXIT_FAILURE) {
    damgr_free_queue(&queue);
    return ret;
  }

  ret = damgr_stage_with_context(&context);
  if (ret == EXIT_FAILURE) {
    damgr_free_queue(&queue);
    return ret;
  }

  ret = damgr_commit();
  if (ret == EXIT_FAILURE) {
    damgr_free_queue(&queue);
    return ret;
  }

  damgr_free_queue(&queue);
  damgr_log(INFO, "finished damgr merge");
  return EXIT_SUCCESS;
}
