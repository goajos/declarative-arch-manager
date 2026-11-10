#ifndef DAMGR_MERGE_H
#define DAMGR_MERGE_H

#include "damgr/cmd.h"
#include "damgr/commit.h"
#include "damgr/context.h"
#include "damgr/journal.h"
#include "damgr/paths.h"
#include "damgr/result.h"
#include "damgr/utils.h"

/* merge calls journal to create the .journal file and gathers tasks from the
 * active and state config diff, runs the task transactions.
 *
 * on success: merge calls commit
 */

/* merge postconditions:
 *   on success (merge calls commit):
 *     - commit postconditions hold (.tmp and .journal don't exist)
 *   on failure:
 *     - .tmp folder doesn't exist
 *     - .journal exists with SUCCEEDED/FAILED/PENDING cmds and no DONE line
 */

/*  merge preconditions:
 *   - .state folder exists
 *   - .tmp folder doesn't exist
 *   - .journal doesn't exist
 * */
Result_t damgr_merge_r(const Damgr_Paths *paths, const Damgr_Context *context);

#ifdef DAMGR_MERGE_IMPLEMENTATION

static Result_t check_merge_preconditions(const Damgr_Paths *paths) {
  bool exists;
  DAMGR_CALL(damgr_make_dir_r(paths->state_dir, &exists));
  DAMGR_CALL(damgr_dir_exists_r(paths->tmp_dir, &exists));
  if (exists) {
    return Error(
        damgr_err_fmt(".tmp folder should not exist: %s", paths->tmp_dir));
  }
  DAMGR_CALL(damgr_reg_exists_r(paths->journal, &exists));
  if (exists) {
    return Error(damgr_err_fmt(".journal should not exist, finish or roll back "
                               "the previous merge first: %s",
                               paths->journal));
  }

  return Success;
}

static const Damgr_Mod EMPTY_MOD = {0};
static const Damgr_Mod *find_mod(const Damgr_Host *host, const char *name) {
  if (host == nullptr) // no state host on the first merge
    return &EMPTY_MOD;
  for (size_t i = 0; i < host->modules_cnt; ++i)
    if (strcmp(host->modules[i]->mod_name, name) == 0)
      return host->modules[i];
  return &EMPTY_MOD;
}

static bool contains(const char *const *names, const bool *flags, size_t cnt,
                     const char *name, bool flag) {
  for (size_t i = 0; i < cnt; ++i)
    if (strcmp(names[i], name) == 0 && (flags == nullptr || flags[i] == flag))
      return true;
  return false;
}

static Result_t push_missing_r(Damgr_Journal *journal, Damgr_Task_Type type,
                               const char *mod_name, const char *const *from,
                               const bool *from_flags, size_t from_cnt,
                               const char *const *in, const bool *in_flags,
                               size_t in_cnt) {
  for (size_t i = 0; i < from_cnt; ++i) {
    bool flag = from_flags != nullptr && from_flags[i];
    if (!contains(in, in_flags, in_cnt, from[i], flag))
      DAMGR_CALL(damgr_journal_push_r(journal, type, mod_name, from[i], flag));
  }
  return Success;
}

static Result_t push_type_r(Damgr_Journal *journal, Damgr_Task_Type type,
                            const Damgr_Mod *state, const Damgr_Mod *active,
                            const char *mod_name) {
  bool removal = type <= PKG_REMOVE; // relies on Damgr_Task_Type order
  const Damgr_Mod *from = removal ? state : active;
  const Damgr_Mod *in = removal ? active : state;

  switch (type) {
  case SVC_DISABLE:
  case SVC_ENABLE:
    return push_missing_r(journal, type, mod_name, from->svc, from->svc_flag,
                          from->svc_cnt, in->svc, in->svc_flag, in->svc_cnt);
  case DOT_UNLINK:
  case DOT_LINK:
    return push_missing_r(journal, type, mod_name, &from->dotfile, nullptr,
                          from->to_link ? 1 : 0, &in->dotfile, nullptr,
                          in->to_link ? 1 : 0);
  case AUR_REMOVE:
  case AUR_INSTALL:
    return push_missing_r(journal, type, mod_name, from->aur, nullptr,
                          from->aur_cnt, in->aur, nullptr, in->aur_cnt);
  case PKG_REMOVE:
  case PKG_INSTALL:
    return push_missing_r(journal, type, mod_name, from->pkg, nullptr,
                          from->pkg_cnt, in->pkg, nullptr, in->pkg_cnt);
  }
  return Success;
}

// fills the journal in execution order: outer loop over the task types
static Result_t build_journal_r(const Damgr_Context *context,
                                Damgr_Journal *journal) {
  const Damgr_Host *active = context->active_host;
  const Damgr_Host *state = context->state_host; // nullptr on first merge

  for (int t = SVC_DISABLE; t <= SVC_ENABLE; ++t) {
    Damgr_Task_Type type = (Damgr_Task_Type)t;

    // added or changed modules
    for (size_t i = 0; i < active->modules_cnt; ++i) {
      const Damgr_Mod *a = active->modules[i];
      DAMGR_CALL(push_type_r(journal, type, find_mod(state, a->mod_name), a,
                             a->mod_name));
    }

    // removed modules
    if (state == nullptr)
      continue;
    for (size_t i = 0; i < state->modules_cnt; ++i) {
      const Damgr_Mod *s = state->modules[i];
      if (find_mod(active, s->mod_name) == &EMPTY_MOD)
        DAMGR_CALL(push_type_r(journal, type, s, &EMPTY_MOD, s->mod_name));
    }
  }
  return Success;
}

static Result_t run_cmd_r(const Damgr_Paths *paths, const char *aur_helper,
                          const Damgr_Cmd *cmd) {
  switch (cmd->type) {
  case SVC_DISABLE:
    return damgr_svc_r(false, cmd->flag, cmd->name);
  case DOT_UNLINK:
    return damgr_dotfile_unlink_r(paths, cmd->name);
  case AUR_REMOVE:
  case PKG_REMOVE:
    return damgr_pkg_r(false, cmd->name);
  case PKG_INSTALL:
    return damgr_pkg_r(true, cmd->name);
  case AUR_INSTALL:
    return damgr_aur_install_r(aur_helper, cmd->name);
  case DOT_LINK:
    return damgr_dotfile_link_r(paths, cmd->name);
  case SVC_ENABLE:
    return damgr_svc_r(true, cmd->flag, cmd->name);
  }
  return Error("unknown task type");
}

static Result_t run_journal_r(const Damgr_Paths *paths, const char *aur_helper,
                              Damgr_Journal *journal) {
  DAMGR_CALL(damgr_journal_write_r(paths, journal)); // all PENDING (or DONE)

  for (size_t i = 0; i < journal->cnt; ++i) {
    Damgr_Cmd *cmd = &journal->cmds[i];
    damgr_log(INFO, "[%zu/%zu] %s\t%s\t%s", i + 1, journal->cnt, cmd->mod_name,
              damgr_task_type_string(cmd->type), cmd->name);

    Result_t cmd_r = run_cmd_r(paths, aur_helper, cmd);
    cmd->status = cmd_r.is_success ? SUCCEEDED : FAILED;
    if (!cmd_r.is_success)
      damgr_log(ERROR, "%s", cmd_r.error);

    DAMGR_CALL(damgr_journal_write_r(paths, journal)); // write on cmd failure
    if (!cmd_r.is_success)
      return Error(damgr_err_fmt(
          "cmd %zu/%zu failed: manually perform the FAILED/PENDING cmds, add "
          "a %s line to %s and run damgr commit",
          i + 1, journal->cnt, JOURNAL_DONE, paths->journal));
  }
  return Success;
}

Result_t damgr_merge_r(const Damgr_Paths *paths, const Damgr_Context *context) {
  DAMGR_CALL(check_merge_preconditions(paths));

  Damgr_Journal journal = {0};
  DAMGR_CALL(build_journal_r(context, &journal));

  Result_t r = run_journal_r(paths, context->aur_helper, &journal);
  damgr_journal_free(&journal);
  if (!r.is_success)
    return r;

  return damgr_commit_r(paths, context);
}

#endif /* DAMGR_MERGE_IMPLEMENTATION */
#endif /* DAMGR_MERGE_H */
