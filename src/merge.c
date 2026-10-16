#include "damgr/merge.h"
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

  Damgr_Context context = {0};
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

  return EXIT_SUCCESS;
}
