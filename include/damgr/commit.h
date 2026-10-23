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
 *   - .tmp folder doesn't exist
 *   - .journal doesn't exist
 * */

/*  commit preconditions:
 *   - (implicit state folder exists)
 *   - .tmp folder doesn't exist
 *   - .journal exists
 *   - .journal status DONE
 * */
Result_t damgr_check_commit_preconditions(Damgr_Paths *paths, struct stat *st);

// TODO: have all the copy file logic inside commit?
#ifdef DAMGR_COMMIT_IMPLEMENTATION

Result_t damgr_check_commit_preconditions(Damgr_Paths *paths, struct stat *st) {
  bool empty;
  DAMGR_CALL(damgr_make_dir_r(paths->tmp_dir, st, &empty));
  DAMGR_CALL(damgr_is_dir_empty_r(paths->tmp_dir, &empty));
  if (!empty) {
    return Error(damgr_err_fmt("%s: %s", paths->tmp_dir, strerror(ENOTEMPTY)));
  }
  return Success;
}

#endif /* DAMGR_COMMIT_IMPLEMENTATION */
#endif /* DAMGR_COMMIT_H */
