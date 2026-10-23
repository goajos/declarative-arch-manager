#ifndef DAMGR_MERGE_H
#define DAMGR_MERGE_H

#include "damgr/paths.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/* merge gathers tasks from the active and state config diff, runs the task
 * transactions and on success runs commit.
 */

/* merge postconditions:
 *   - .state folder exists
 *   - .state folder contains 1 file for every active config/host/module file
 *   - .tmp folder doesn't exist
 *   - .journal exist
 *   - .journal status DONE
 */

/*  merge preconditions:
 *   - .state folder exists
 *   - .tmp folder doesn't exist
 *   - .journal exists
 *   - .journal status PENDING
 * */
Result_t damgr_check_merge_preconditions(Damgr_Paths *paths, struct stat *st);

#ifdef DAMGR_MERGE_IMPLEMENTATION

Result_t damgr_check_merge_preconditions(Damgr_Paths *paths, struct stat *st) {
  bool empty;
  bool exists;
  DAMGR_CALL(damgr_make_dir_r(paths->state_dir, st, &empty));
  DAMGR_CALL(damgr_dir_exists_r(paths->tmp_dir, st, &exists));
  if (exists) {
    return Error(
        damgr_err_fmt(".tmp folder should not exist: %s", paths->tmp_dir));
  }
  return Success;
}

#endif /* DAMGR_MERGE_IMPLEMENTATION */
#endif /* DAMGR_MERGE_H */
