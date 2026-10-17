#include "damgr/merge.h"
#include "damgr/commit.h"
#include "damgr/diff.h"
#include "damgr/stage.h"
#include "damgr/task.h"
#include "damgr/transaction.h"
#include "damgr/utils.h"

int damgr_merge(Damgr_Arena *arena) {

  int ret;
  char fid_buf[DAMGR_FID_BUF_MAX];
  snprintf(fid_buf, sizeof(fid_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  // state folder should be created before merge
  if (!damgr_make_dir(fid_buf)) {
    return EXIT_FAILURE;
  }

  snprintf(fid_buf, sizeof(fid_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STAGING));
  // staging folder shouldn't exist before merge
  if (damgr_dir_exists(fid_buf)) {
    damgr_log(ERROR, "trying to merge with a .staging folder");
    return EXIT_FAILURE;
  }

  snprintf(fid_buf, sizeof(fid_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(BACKUP));
  // backup folder shouldn't exist before merge
  if (damgr_dir_exists(fid_buf)) {
    damgr_log(ERROR, "trying to merge with a backup folder");
    return EXIT_FAILURE;
  }

  Damgr_Context context = {nullptr};
  snprintf(fid_buf, sizeof(fid_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  if (damgr_is_dir_empty(fid_buf)) {
    damgr_log(INFO, "no state to parse, the state directory is empty: %s",
              fid_buf);
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

  ret = damgr_transaction(context.aur_helper, &queue);
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
