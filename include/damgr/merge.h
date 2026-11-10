#ifndef DAMGR_MERGE_H
#define DAMGR_MERGE_H

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
 *   - .tmp folder doesn't exist
 *   - .journal exists
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

// TODO: this can be removed in phase 6?
static void log_journal(const Damgr_Journal *journal) {
  damgr_log(INFO, "journal contains %zu cmd(s)", journal->cnt);
  for (size_t i = 0; i < journal->cnt; ++i) {
    const Damgr_Cmd *cmd = &journal->cmds[i];
    damgr_log(INFO, "%s\t%s\t%s%s", cmd->mod_name,
              damgr_task_type_string(cmd->type), cmd->name,
              (cmd->type == SVC_ENABLE || cmd->type == SVC_DISABLE)
                  ? (cmd->flag ? ":true" : ":false")
                  : "");
  }
}

Result_t damgr_merge_r(const Damgr_Paths *paths, const Damgr_Context *context) {
  DAMGR_CALL(check_merge_preconditions(paths));

  Damgr_Journal journal = {0};
  // save result so the journal is always freed before return
  Result_t r = build_journal_r(context, &journal);
  if (r.is_success)
    log_journal(&journal); // phase 5: inspect only, phase 6 runs the cmds
  damgr_journal_free(&journal);
  return r;
}

#endif /* DAMGR_MERGE_IMPLEMENTATION */
#endif /* DAMGR_MERGE_H */
