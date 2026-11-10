#ifndef DAMGR_COMMIT_H
#define DAMGR_COMMIT_H

#include "damgr/context.h"
#include "damgr/journal.h"
#include "damgr/paths.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/* commit copies the state files into the .tmp folder before copying the
 * active files into the state folder.
 *
 * on success: commit will clean up the .tmp folder and calls
 * damgr_journal_delete_r
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
Result_t damgr_commit_r(const Damgr_Paths *paths, const Damgr_Context *context);

#ifdef DAMGR_COMMIT_IMPLEMENTATION

static Result_t check_commit_preconditions_r(const Damgr_Paths *paths) {
  bool exists;
  DAMGR_CALL(damgr_dir_exists_r(paths->tmp_dir, &exists));
  if (exists) {
    return Error(
        damgr_err_fmt(".tmp folder should not exist: %s", paths->tmp_dir));
  }
  DAMGR_CALL(damgr_reg_exists_r(paths->journal, &exists));
  if (!exists) {
    return Error(damgr_err_fmt(".journal should exist: %s", paths->journal));
  }

  bool done;
  DAMGR_CALL(damgr_journal_is_done_r(paths, &done));
  if (!done)
    return Error(damgr_err_fmt(".journal is not done, manually perform the "
                               "FAILED/PENDING cmds and add a %s line EOF: %s",
                               JOURNAL_DONE, paths->journal));

  return Success;
}

static Result_t copy_active_to_state_r(const Damgr_Paths *paths,
                                       const Damgr_Host *host) {
  char src[PATH_BUFFER_MAX];
  char dst[PATH_BUFFER_MAX];

  DAMGR_CALL(damgr_copy_file_r(paths->active_config, paths->state_config));

  DAMGR_CALL(damgr_resolve_host_path_r(paths, src, sizeof(src), host->host_name,
                                       false));
  DAMGR_CALL(damgr_resolve_host_path_r(paths, dst, sizeof(dst), host->host_name,
                                       true));
  DAMGR_CALL(damgr_copy_file_r(src, dst));

  for (size_t i = 0; i < host->modules_cnt; ++i) {
    const char *name = host->modules[i]->mod_name;
    DAMGR_CALL(
        damgr_resolve_module_path_r(paths, src, sizeof(src), name, false));
    DAMGR_CALL(
        damgr_resolve_module_path_r(paths, dst, sizeof(dst), name, true));
    DAMGR_CALL(damgr_copy_file_r(src, dst));
  }
  return Success;
}

static Result_t replace_state_r(const Damgr_Paths *paths,
                                const Damgr_Host *host) {
  DAMGR_CALL(damgr_remove_dir_files_r(paths->state_dir));
  DAMGR_CALL(copy_active_to_state_r(paths, host));
  return Success;
}

static Result_t restore_state_r(const Damgr_Paths *paths) {
  DAMGR_CALL(damgr_remove_dir_files_r(paths->state_dir));
  DAMGR_CALL(damgr_copy_dir_files_r(paths->tmp_dir, paths->state_dir));
  DAMGR_CALL(damgr_remove_dir_r(paths->tmp_dir));
  return Success;
}

Result_t damgr_commit_r(const Damgr_Paths *paths,
                        const Damgr_Context *context) {
  DAMGR_CALL(check_commit_preconditions_r(paths));

  bool exists;
  DAMGR_CALL(damgr_make_dir_r(paths->tmp_dir, &exists));
  Result_t r = damgr_copy_dir_files_r(paths->state_dir, paths->tmp_dir);
  if (!r.is_success) {
    damgr_log(ERROR, "%s", r.error);
    damgr_remove_dir_r(paths->tmp_dir);
    return Error("could not back up the state files into .tmp");
  }

  r = replace_state_r(paths, context->active_host);
  if (!r.is_success) {
    damgr_log(ERROR, "%s", r.error);
    r = restore_state_r(paths);
    if (!r.is_success) {
      damgr_log(ERROR, "%s", r.error);
      return Error(damgr_err_fmt("restore failed: manually copy %s/* into %s "
                                 "and remove %s",
                                 paths->tmp_dir, paths->state_dir,
                                 paths->tmp_dir));
    }
    return Error("state files restored from .tmp, .journal kept");
  }

  DAMGR_CALL(damgr_remove_dir_r(paths->tmp_dir));
  DAMGR_CALL(damgr_journal_delete_r(paths));
  return Success;
}

#endif /* DAMGR_COMMIT_IMPLEMENTATION */
#endif /* DAMGR_COMMIT_H */
