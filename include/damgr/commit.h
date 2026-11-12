#ifndef DAMGR_COMMIT_H
#define DAMGR_COMMIT_H

#include "damgr/context.h"
#include "damgr/path.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/* commit copies the state files into the .tmp folder before copying the
 * active files into the state folder.
 *
 * on success: commit will clean up the .tmp folder and delete the journal
 */

/* commit postconditions:
 *   - .state folder contains 1 file for every active config/host/module file
 *   - .tmp folder doesn't exist
 *   - .journal doesn't exist
 * */

/*  commit preconditions:
 *   - .tmp folder doesn't exist
 *   - .journal exists
 *   - .journal status DONE
 * */
Result_t damgr_commit_preconditions_result(const Damgr_Paths *paths);
Result_t damgr_commit_result(const Damgr_Paths *paths,
                             const Damgr_Context *context);

#ifdef DAMGR_COMMIT_IMPLEMENTATION

Result_t damgr_commit_preconditions_result(const Damgr_Paths *paths) {
  bool exists;
  DAMGR_CALL(damgr_dir_exists_result(paths->tmp_dir, &exists));
  if (exists) {
    return Error(".tmp folder should not exist: %s", paths->tmp_dir);
  }

  DAMGR_CALL(damgr_reg_exists_result(paths->journal, &exists));
  if (!exists)
    return Error(".journal should exist: %s", paths->journal);

  // TODO: check if journal status is DONE

  return Success;
}

// Result_t damgr_commit_result(const Damgr_Paths *paths,
//                              const Damgr_Context *context) {
//   return Success;
// }

#endif /* DAMGR_COMMIT_IMPLEMENTATION */
#endif /* DAMGR_COMMIT_H */
