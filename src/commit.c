#define _DEFAULT_SOURCE
#include "damgr/commit.h"

static int backup_state() {
  char state_buf[DAMGR_FID_BUF_MAX];
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  char backup_buf[DAMGR_FID_BUF_MAX];
  snprintf(backup_buf, sizeof(backup_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(BACKUP));

  return damgr_copy_dir_except(state_buf, backup_buf, false, DAMGR_JOURNAL);
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

  if (damgr_clean_dir_except(state_buf, true, DAMGR_JOURNAL) != EXIT_SUCCESS) {
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

static int journal_guard(const char *state_buf) {
  char journal_buf[DAMGR_FID_BUF_MAX + sizeof(DAMGR_JOURNAL)];
  snprintf(journal_buf, sizeof(journal_buf), "%s/%s", state_buf, DAMGR_JOURNAL);
  int phase = -1;
  int ntasks = -1;
  int ret;
  ret = damgr_read_journal(journal_buf, &phase, &ntasks);
  if (ret == EXIT_FAILURE)
    return ret; // can't opeh or malformed journal

  if (phase != DAMGR_JOURNAL_DONE) {
    damgr_log(ERROR,
              "journal %s has phase %s from an interrupted transaction, "
              "refusing to commit, inspect the journal and clean up the "
              "residual tasks manually",
              journal_buf, journal_phase_string((Damgr_Journal_Phase)phase));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
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

  if (journal_guard(state_buf) == EXIT_FAILURE)
    return EXIT_FAILURE;

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
    if (damgr_clean_dir_except(state_buf, true, DAMGR_JOURNAL) ==
        EXIT_FAILURE) {
      restore_from_backup();
      return EXIT_FAILURE;
    }
  }

  if (rename_staging() == EXIT_FAILURE) {
    restore_from_backup();
    return EXIT_FAILURE;
  }

  if (damgr_clean_dir(staging_buf, false) == EXIT_FAILURE)
    return EXIT_FAILURE;

  if (damgr_dir_exists(backup_buf) &&
      damgr_clean_dir(backup_buf, false) == EXIT_FAILURE)
    return EXIT_FAILURE;

  char journal_buf[DAMGR_FID_BUF_MAX + sizeof(DAMGR_JOURNAL)];
  snprintf(journal_buf, sizeof(journal_buf), "%s/%s", state_buf, DAMGR_JOURNAL);
  int phase = -1;
  int ntasks = -1;
  if (damgr_read_journal(journal_buf, &phase, &ntasks) == EXIT_SUCCESS &&
      damgr_clean_journal(journal_buf) != EXIT_SUCCESS) {
    damgr_log(ERROR, "could not clear journal after successful commit: %s",
              journal_buf);
  }

  damgr_log(INFO, "finished damgr commit");
  return EXIT_SUCCESS;
}
