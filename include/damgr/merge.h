#ifndef DAMGR_MERGE_H
#define DAMGR_MERGE_H

#include "damgr/commit.h"
#include "damgr/context.h"
#include "damgr/journal.h"
#include "damgr/path.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/* merge creates the journal and gathers tasks and runs the task transactions.
 */

/* merge postconditions:
 *   on success:
 *    - journal exists with status DONE
 *   on failure:
 *    - journal exists without status DONE
 */

/*  merge preconditions:
 *   - .tmp folder doesn't exist
 *   - .journal doesn't exist
 * */
Result_t damgr_merge_preconditions_result(const Damgr_Paths *paths);
Result_t damgr_merge_result(const Damgr_Paths *paths,
                            const Damgr_Context *context);

#ifdef DAMGR_MERGE_IMPLEMENTATION

Result_t damgr_merge_preconditions_result(const Damgr_Paths *paths) {
  bool exists;
  DAMGR_CALL(damgr_dir_exists_result(paths->tmp_dir, &exists));
  if (exists)
    return Error(".tmp folder should not exist: %s", paths->tmp_dir);

  DAMGR_CALL(damgr_reg_exists_result(paths->journal, &exists));
  if (exists)
    return Error(".journal should not exist: %s", paths->journal);

  return Success;
}

// Result_t damgr_merge_result(const Damgr_Paths *paths,
//                             const Damgr_Context *context) {
//
//   return Success;
// }

#endif /* DAMGR_MERGE_IMPLEMENTATION */
#endif /* DAMGR_MERGE_H */
