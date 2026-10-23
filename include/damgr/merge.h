#ifndef DAMGR_MERGE_H
#define DAMGR_MERGE_H

#include "damgr/paths.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/*
 * merge postconditions:
 *   -
 * */

/*  merge preconditions:
 *    - .state folder should exist
 *    - .staging folder should not exist
 *    - .backup folder should not exist
 * */
Result_t damgr_check_merge_preconditions(Damgr_Paths *paths, struct stat *st);

#ifdef DAMGR_MERGE_IMPLEMENTATION

Result_t damgr_check_merge_preconditions(Damgr_Paths *paths, struct stat *st) {
  bool empty;
  bool exists;
  DAMGR_CALL(damgr_make_dir_r(paths->state_dir, st, &empty));
  DAMGR_CALL(damgr_dir_exists_r(paths->staging_dir, st, &exists));
  if (exists) {
    return Error(damgr_err_fmt(".staging folder should not exist: %s",
                               paths->staging_dir));
  }
  DAMGR_CALL(damgr_dir_exists_r(paths->backup_dir, st, &exists));
  if (exists) {
    return Error(damgr_err_fmt(".backup folder should not exist: %s",
                               paths->backup_dir));
  }
  return Success;
}

#endif /* DAMGR_MERGE_IMPLEMENTATION */
#endif /* DAMGR_MERGE_H */
