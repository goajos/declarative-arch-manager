#ifndef DAMGR_MERGE_H
#define DAMGR_MERGE_H

#include "damgr/paths.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/* merge calls journal to create the .journal file and gathers tasks from the
 * active and state config diff, runs the task transactions.
 *
 * on success: merge calls commit
 */

/* merge postconditions:
 *   - .tmp folder doesn't exist
 *   - .journal exists
 */

/*  merge preconditions:
 *   - .state folder exists
 *   - .tmp folder doesn't exist
 *   - .journal doesn't exist
 * */
Result_t damgr_check_merge_preconditions(Damgr_Paths *paths, struct stat *st);

// TODO: have all the diff + transaction logic inside merge?
#ifdef DAMGR_MERGE_IMPLEMENTATION

Result_t damgr_check_merge_preconditions(Damgr_Paths *paths, struct stat *st) {
  bool existed;
  DAMGR_CALL(damgr_make_dir_r(paths->state_dir, st, &existed));
  bool exists;
  DAMGR_CALL(damgr_dir_exists_r(paths->tmp_dir, st, &exists));
  if (exists) {
    return Error(
        damgr_err_fmt(".tmp folder should not exist: %s", paths->tmp_dir));
  }
  DAMGR_CALL(damgr_reg_exists_r(paths->journal, st, &exists));
  if (exists) {
    return Error(damgr_err_fmt(".journal should not exist, finish or roll back "
                               "the previous merge first: %s",
                               paths->journal));
  }

  return Success;
}

#endif /* DAMGR_MERGE_IMPLEMENTATION */
#endif /* DAMGR_MERGE_H */
