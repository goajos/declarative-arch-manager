#ifndef DAMGR_COMMIT_H
#define DAMGR_COMMIT_H

#include "damgr/paths.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/* commit copies the state files into the .backup folder before copying the
 * active files into the state folder.
 * commit postconditions:
 *   -
 * */

/*  commit preconditions:
 *    - (implicit state folder exists)
 *    - .staging folder should not be empty
 *    - .backup folder should exist
 *    - .backup folder should be empty
 * */
Result_t damgr_check_commit_preconditions(Damgr_Paths *paths, struct stat *st);

#ifdef DAMGR_COMMIT_IMPLEMENTATION

Result_t damgr_check_commit_preconditions(Damgr_Paths *paths, struct stat *st) {
  bool empty;
  DAMGR_CALL(damgr_is_dir_empty_r(paths->staging_dir, &empty));
  if (empty) {
    return Error(
        damgr_err_fmt(".staging folder is empty: %s", paths->staging_dir));
  }
  DAMGR_CALL(damgr_make_dir_r(paths->backup_dir, st, &empty));
  DAMGR_CALL(damgr_is_dir_empty_r(paths->backup_dir, &empty));
  if (!empty) {
    return Error(
        damgr_err_fmt("%s: %s", paths->backup_dir, strerror(ENOTEMPTY)));
  }
  return Success;
}

#endif /* DAMGR_COMMIT_IMPLEMENTATION */
#endif /* damgr_commit_h */
