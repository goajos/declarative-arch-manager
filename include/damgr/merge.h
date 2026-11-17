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

typedef struct {
  const char *const *names;
  const bool *flags;
  size_t cnt;
} Items;

static Items mod_items(const Damgr_Mod *mod, const Damgr_Task_Type type) {
  switch (type) {
  case SVC_DISABLE:
  case SVC_ENABLE:
    // flag = scope (root/user)
    return (Items){mod->svc, mod->svc_flag, mod->svc_cnt};
  case DOT_UNLINK:
  case DOT_LINK:
    // to_link == false is equivalent to no dotfile item in the module
    return (Items){&mod->dotfile, nullptr, mod->to_link ? 1 : 0};
  case AUR_REMOVE:
  case AUR_INSTALL:
    return (Items){mod->aur, nullptr, mod->aur_cnt};
  case PKG_REMOVE:
  case PKG_INSTALL:
    return (Items){mod->pkg, nullptr, mod->pkg_cnt};
  }
  return (Items){0};
}

static bool items_contain(const Items items, const char *name, bool flag) {
  for (size_t i = 0; i < items.cnt; ++i) {
    if (strcmp(items.names[i], name) == 0 &&
        (items.flags == nullptr || items.flags[i] == flag))
      return true;
  }
  return false;
}

// true if any module of host has name (+flag) for this task type
static bool host_contains(const Damgr_Host *host, const Damgr_Task_Type type,
                          const char *name, bool flag) {
  if (host == nullptr)
    return false;
  for (size_t i = 0; i < host->mods_cnt; ++i) {
    if (items_contain(mod_items(host->mods[i], type), name, flag))
      return true;
  }
  return false;
}

// true if this task is already queued
static bool journal_contains(const Damgr_Task_Type type, const char *name,
                             bool flag) {
  for (size_t i = 0; i < damgr_journal.cnt; ++i) {
    const Damgr_Cmd *cmd = &damgr_journal.cmds[i];
    if (cmd->type == type && cmd->flag == flag && strcmp(cmd->name, name) == 0)
      return true;
  }
  return false;
}

static Result_t get_tasks_result(const Damgr_Context *context) {
  const Damgr_Host *ahost = context->active_host;
  const Damgr_Host *shost = context->state_host; // nullptr on first merge

  for (int t = SVC_DISABLE; t <= SVC_ENABLE; ++t) {
    Damgr_Task_Type type = (Damgr_Task_Type)t;
    bool negative = type <= PKG_REMOVE;
    // negative: in state but not in active, positive: in active but not state
    const Damgr_Host *out = negative ? shost : ahost;
    if (out == nullptr)
      continue;
    const Damgr_Host *in = negative ? ahost : shost;

    for (size_t i = 0; i < out->mods_cnt; ++i) {
      const Damgr_Mod *mod = out->mods[i];
      Items items = mod_items(mod, type);
      for (size_t j = 0; j < items.cnt; ++j) {
        const char *name = items.names[j];
        bool flag = items.flags != nullptr && items.flags[j];
        if (host_contains(in, type, name, flag) ||
            journal_contains(type, name, flag))
          continue;
        DAMGR_CALL(damgr_journal_push_result(type, mod->mod_name, name, flag));
      }
    }
  }

  return Success;
}

Result_t damgr_merge_result([[maybe_unused]] const Damgr_Paths *paths,
                            const Damgr_Context *context) {
  DAMGR_CALL(get_tasks_result(context));

  return Success;
}

#endif /* DAMGR_MERGE_IMPLEMENTATION */
#endif /* DAMGR_MERGE_H */
