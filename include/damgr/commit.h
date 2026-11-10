#ifndef DAMGR_COMMIT_H
#define DAMGR_COMMIT_H

#include "damgr/paths.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/* commit copies the state files into the .tmp folder before copying the
 * active files into the state folder.
 *
 * on success: commit will clean up the .tmp folder and call journal to clean up
 * the .journal file.
 */

/* commit postconditions:
 *   - .state folder contains 1 file for every active config/host/module file
 *   - .tmp folder doesn't exist
 *   - .journal doesn't exist
 * */

/*  commit preconditions:
 *   - state folder exists
 *   - .tmp folder doesn't exist
 *   - .journal exists
 *   - .journal status DONE
 * */
Result_t damgr_check_commit_preconditions(Damgr_Paths *paths, struct stat *st);

// TODO: have all the copy file logic inside commit?
#ifdef DAMGR_COMMIT_IMPLEMENTATION

Result_t damgr_check_commit_preconditions(Damgr_Paths *paths, struct stat *st) {
  bool exists;
  DAMGR_CALL(damgr_dir_exists_r(paths->state_dir, st, &exists));
  if (!exists) {
    return Error(
        damgr_err_fmt(".state folder should exist: %s", paths->state_dir));
  }
  DAMGR_CALL(damgr_dir_exists_r(paths->tmp_dir, st, &exists));
  if (exists) {
    return Error(
        damgr_err_fmt(".tmp folder should not exist: %s", paths->tmp_dir));
  }
  DAMGR_CALL(damgr_reg_exists_r(paths->journal, st, &exists));
  if (!exists) {
    return Error(damgr_err_fmt(".journal should exist: %s", paths->journal));
  }

  // TODO: .journal status DONE (every subcommand SUCCEEDED)

  return Success;
}

#endif /* DAMGR_COMMIT_IMPLEMENTATION */
#endif /* DAMGR_COMMIT_H */
