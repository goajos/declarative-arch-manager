#define _DEFAULT_SOURCE
#include "damgr/commit.h"
#include "damgr/utils.h"

static int backup_state() {
  char state_buf[DAMGR_FID_BUF_MAX];
  char backup_buf[DAMGR_FID_BUF_MAX];
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  snprintf(backup_buf, sizeof(backup_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(BACKUP));

  return damgr_copy_dir(state_buf, backup_buf, false);
}

static int rename_staging() {
  char staging_buf[DAMGR_FID_BUF_MAX];
  char state_buf[DAMGR_FID_BUF_MAX];
  snprintf(staging_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STAGING));
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));

  return damgr_copy_dir(staging_buf, state_buf, true);
}

int damgr_commit() {
  char state_buf[DAMGR_FID_BUF_MAX];
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  // state folder should exist before commit
  if (!damgr_dir_exists(state_buf)) {
    damgr_log(ERROR, "trying to commit without a state folder");
    return EXIT_FAILURE;
  }

  char staging_buf[DAMGR_FID_BUF_MAX];
  // staging folder should exist before commit
  snprintf(staging_buf, sizeof(staging_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STAGING));
  if (!damgr_dir_exists(staging_buf)) {
    damgr_log(ERROR, "trying to commit without a .staging folder");
    return EXIT_FAILURE;
  }
  // staging folder shouldn't be empty before commit
  if (damgr_is_dir_empty(staging_buf)) {
    damgr_log(ERROR, "trying to commit with an empty .staging folder");
    return EXIT_FAILURE;
  }

  // state folder can be empty before commit with no state merge
  char backup_buf[DAMGR_FID_BUF_MAX];
  if (!damgr_is_dir_empty(state_buf)) {
    snprintf(backup_buf, sizeof(backup_buf), "%s/%s", getenv("HOME"),
             state_dir_type_string(BACKUP));
    // TODO: do we also want to clear the dir here?
    if (damgr_make_dir(backup_buf) == EXIT_FAILURE)
      return EXIT_FAILURE;

    if (backup_state() == EXIT_FAILURE)
      return EXIT_FAILURE;
  }

  if (rename_staging() == EXIT_FAILURE)
    return EXIT_FAILURE;

  if (damgr_remove_dir_recursive(staging_buf) == EXIT_FAILURE)
    return EXIT_FAILURE;

  if (damgr_remove_dir_recursive(backup_buf) == EXIT_FAILURE)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
