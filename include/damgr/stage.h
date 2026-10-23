#ifndef DAMGR_STAGE_H
#define DAMGR_STAGE_H

#include "damgr/paths.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/* stage copies the active config into the .staging folder
 * stage postconditions:
 *   -
 * */

/*  stage preconditions:
 *    - (implicit state folder exists)
 *    - .staging folder should exist
 *    - .staging folder should be empty
 * */
Result_t damgr_check_stage_preconditions(Damgr_Paths *paths, struct stat *st);

#ifdef DAMGR_STAGE_IMPLEMENTATION

Result_t damgr_check_stage_preconditions(Damgr_Paths *paths, struct stat *st) {
  bool empty;
  DAMGR_CALL(damgr_make_dir_r(paths->staging_dir, st, &empty));
  DAMGR_CALL(damgr_is_dir_empty_r(paths->staging_dir, &empty));
  if (empty) {
    return Error(
        damgr_err_fmt("%s: %s", paths->staging_dir, strerror(ENOTEMPTY)));
  }
  return Success;
}

#endif /* DAMGR_STAGE_IMPLEMENTATION */
#endif /* DAMGR_STAGE_H */
