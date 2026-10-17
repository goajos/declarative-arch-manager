#define _DEFAULT_SOURCE
#include "damgr/commit.h"
#include "damgr/utils.h"

static int backup_state() {
  char state_buf[DAMGR_FID_BUF_MAX];
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  char backup_buf[DAMGR_FID_BUF_MAX];
  snprintf(backup_buf, sizeof(backup_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(BACKUP));

  return damgr_copy_dir(state_buf, backup_buf, false);
}

static int rename_staging() {
  char staging_buf[DAMGR_FID_BUF_MAX];
  snprintf(staging_buf, sizeof(staging_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STAGING));
  char state_buf[DAMGR_FID_BUF_MAX];
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));

  return damgr_copy_dir(staging_buf, state_buf, true);
}

static int restore_from_backup() {
  char state_buf[DAMGR_FID_BUF_MAX];
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  char backup_buf[DAMGR_FID_BUF_MAX];
  snprintf(backup_buf, sizeof(backup_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(BACKUP));

  if (!damgr_dir_exists(backup_buf)) {
    // nothing to restore
    return EXIT_SUCCESS;
  }

  damgr_log(WARNING, "commit failed: restoring system state from %s",
            backup_buf);

  if (damgr_clean_dir(state_buf, true) != EXIT_SUCCESS) {
    damgr_log(ERROR,
              "failed to clean state dir: manually clean corrupted state dir "
              "and move files from %s -> %s",
              backup_buf, state_buf);
    return EXIT_FAILURE;
  }

  if (damgr_copy_dir(backup_buf, state_buf, false) != EXIT_SUCCESS) {
    damgr_log(ERROR,
              "failed to copy backup files into state dir: manually move files "
              "from %s -> %s",
              backup_buf, state_buf);
    return EXIT_FAILURE;
  }

  if (damgr_clean_dir(backup_buf, false) != EXIT_SUCCESS) {
    damgr_log(ERROR,
              "system state restored, but failed to clean %s - manual cleanup "
              "required!",
              backup_buf);
    return EXIT_FAILURE;
  }

  damgr_log(INFO, "successfully restored system state from backup");
  return EXIT_SUCCESS;
}

// TODO: better error handling?
int damgr_commit() {
  char state_buf[DAMGR_FID_BUF_MAX];
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  // state folder should exist before commit
  if (!damgr_dir_exists(state_buf)) {
    damgr_log(ERROR, "trying to commit without a state folder");
    return EXIT_FAILURE;
  }

  // TODO: check journal? refuse to commit?

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
  snprintf(backup_buf, sizeof(backup_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(BACKUP));
  if (!damgr_is_dir_empty(state_buf)) {
    if (damgr_dir_exists(backup_buf) && !damgr_is_dir_empty(backup_buf)) {
      damgr_log(ERROR, "trying to commit with a non-empty .backup folder");
      return EXIT_FAILURE;
    }

    if (!damgr_make_dir(backup_buf))
      return EXIT_FAILURE;

    if (backup_state() == EXIT_FAILURE)
      return EXIT_FAILURE;

    // destructive phase: restore required
    // TODO: do not remove journal?
    if (damgr_clean_dir(state_buf, true) == EXIT_FAILURE) {
      restore_from_backup();
      return EXIT_FAILURE;
    }
  }

  if (rename_staging() == EXIT_FAILURE) {
    // TODO: how to use the backup folder to restore state?
    restore_from_backup();
    return EXIT_FAILURE;
  }

  if (damgr_clean_dir(staging_buf, false) == EXIT_FAILURE)
    return EXIT_FAILURE;

  if (damgr_dir_exists(backup_buf) &&
      damgr_clean_dir(backup_buf, false) == EXIT_FAILURE)
    return EXIT_FAILURE;

  // TODO: resolve journal done?

  damgr_log(INFO, "finished damgr commit");
  return EXIT_SUCCESS;
}
