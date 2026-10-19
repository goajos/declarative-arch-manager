#define _DEFAULT_SOURCE
#include "damgr/commit.h"

static int backup_state(const Damgr_Paths *paths) {
  return damgr_copy_dir_except(paths->state_dir, paths->backup_dir, false,
                               DAMGR_JOURNAL);
}

static int rename_staging(const Damgr_Paths *paths) {
  return damgr_copy_dir(paths->staging_dir, paths->state_dir, true);
}

static int restore_from_backup(const Damgr_Paths *paths) {
  if (!damgr_dir_exists(paths->backup_dir)) {
    // nothing to restore
    return EXIT_SUCCESS;
  }

  damgr_log(WARNING, "commit failed: restoring system state from %s",
            paths->backup_dir);

  if (damgr_clean_dir_except(paths->state_dir, true, DAMGR_JOURNAL) !=
      EXIT_SUCCESS) {
    damgr_log(ERROR,
              "failed to clean state dir: manually clean corrupted state dir "
              "and move files from %s -> %s",
              paths->backup_dir, paths->state_dir);
    return EXIT_FAILURE;
  }

  if (damgr_copy_dir(paths->backup_dir, paths->state_dir, false) !=
      EXIT_SUCCESS) {
    damgr_log(ERROR,
              "failed to copy backup files into state dir: manually move files "
              "from %s -> %s",
              paths->backup_dir, paths->state_dir);
    return EXIT_FAILURE;
  }

  if (damgr_clean_dir(paths->backup_dir, false) != EXIT_SUCCESS) {
    damgr_log(ERROR,
              "system state restored, but failed to clean %s - manual cleanup "
              "required!",
              paths->backup_dir);
    return EXIT_FAILURE;
  }

  damgr_log(INFO, "successfully restored system state from backup");
  return EXIT_SUCCESS;
}

static int journal_guard(const Damgr_Paths *paths) {
  int phase = -1;
  int ntasks = -1;
  int ret;
  ret = damgr_read_journal(paths->journal, &phase, &ntasks);
  if (ret == EXIT_FAILURE)
    return ret; // can't opeh or malformed journal

  if (phase != DAMGR_JOURNAL_DONE) {
    damgr_log(ERROR,
              "journal %s has phase %s from an interrupted transaction, "
              "refusing to commit, inspect the journal and clean up the "
              "residual tasks manually",
              paths->journal, journal_phase_string((Damgr_Journal_Phase)phase));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int damgr_commit(const Damgr_Paths *paths) {
  // state folder should exist before commit
  if (!damgr_dir_exists(paths->state_dir)) {
    damgr_log(ERROR, "trying to commit without a state folder");
    return EXIT_FAILURE;
  }

  if (journal_guard(paths) == EXIT_FAILURE)
    return EXIT_FAILURE;

  if (!damgr_dir_exists(paths->staging_dir)) {
    damgr_log(ERROR, "trying to commit without a .staging folder");
    return EXIT_FAILURE;
  }
  // staging folder shouldn't be empty before commit
  if (damgr_is_dir_empty(paths->staging_dir)) {
    damgr_log(ERROR, "trying to commit with an empty .staging folder");
    return EXIT_FAILURE;
  }

  // state folder can be empty before commit with no state merge
  if (!damgr_is_dir_empty(paths->state_dir)) {
    if (damgr_dir_exists(paths->backup_dir) &&
        !damgr_is_dir_empty(paths->backup_dir)) {
      damgr_log(ERROR, "trying to commit with a non-empty .backup folder");
      return EXIT_FAILURE;
    }

    if (!damgr_make_dir(paths->backup_dir))
      return EXIT_FAILURE;

    if (backup_state(paths) == EXIT_FAILURE)
      return EXIT_FAILURE;

    // destructive phase: restore required
    if (damgr_clean_dir_except(paths->state_dir, true, DAMGR_JOURNAL) ==
        EXIT_FAILURE) {
      restore_from_backup(paths);
      return EXIT_FAILURE;
    }
  }

  if (rename_staging(paths) == EXIT_FAILURE) {
    restore_from_backup(paths);
    return EXIT_FAILURE;
  }

  if (damgr_clean_dir(paths->staging_dir, false) == EXIT_FAILURE)
    return EXIT_FAILURE;

  if (damgr_dir_exists(paths->backup_dir) &&
      damgr_clean_dir(paths->backup_dir, false) == EXIT_FAILURE)
    return EXIT_FAILURE;

  int phase = -1;
  int ntasks = -1;
  if (damgr_read_journal(paths->journal, &phase, &ntasks) == EXIT_SUCCESS &&
      damgr_clean_journal(paths->journal) != EXIT_SUCCESS) {
    damgr_log(ERROR, "could not clear journal after successful commit: %s",
              paths->journal);
  }

  damgr_log(INFO, "finished damgr commit");
  return EXIT_SUCCESS;
}
