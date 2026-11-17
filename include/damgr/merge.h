#ifndef DAMGR_MERGE_H
#define DAMGR_MERGE_H
#include <sys/wait.h>

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

static Result_t exec_result(char *const argv[]) {
  // return Success;
  fflush(nullptr); // don't duplicate buffered output in the child
  pid_t pid = fork();
  if (pid == -1)
    return Error("fork failed: %s", strerror(errno));
  if (pid == 0) {
    execvp(argv[0], argv);
    _exit(127); // only reached if execvp failed
  }
  int wstatus;
  while (waitpid(pid, &wstatus, 0) == -1)
    if (errno != EINTR)
      return Error("waitpid failed: %s", strerror(errno));
  if (WIFEXITED(wstatus)) {
    if (WEXITSTATUS(wstatus) == EXIT_SUCCESS)
      return Success;
    return Error("%s exited with code %d", argv[0], WEXITSTATUS(wstatus));
  }
  if (WIFSIGNALED(wstatus))
    return Error("%s was terminated by signal %d", argv[0], WTERMSIG(wstatus));

  return Error("%s ended in unexpected state (wait status 0x%x)", argv[0],
               wstatus);
}

static Result_t is_link_result(const char *dst, const char *src, bool *exists,
                               bool *is_link) {
  struct stat st;
  *exists = false;
  *is_link = false;
  if (lstat(dst, &st) != 0) {
    if (errno == ENOENT)
      return Success;
    return Error("lstat %s failed: %s", dst, strerror(errno));
  }
  *exists = true;
  if (!S_ISLNK(st.st_mode))
    return Success;

  char link[DAMGR_PATH_BUFFER_MAX];
  ssize_t len = readlink(dst, link, sizeof(link) - 1);
  if (len < 0)
    return Error("could not read link %s: %s", dst, strerror(errno));

  link[len] = '\0';
  *is_link = strcmp(link, src) == 0;
  return Success;
}

static Result_t run_cmd_result(const Damgr_Paths *paths,
                               const Damgr_Context *context,
                               const Damgr_Cmd *cmd) {
  char *name = (char *)cmd->name;
  switch (cmd->type) {
  case SVC_DISABLE:
  case SVC_ENABLE: {
    char *action = cmd->type == SVC_ENABLE ? "enable" : "disable";
    if (cmd->flag)
      return exec_result(
          (char *[]){"sudo", "systemctl", "--now", action, name, nullptr});

    return exec_result(
        (char *[]){"systemctl", "--user", "--now", action, name, nullptr});
  }
  case DOT_UNLINK:
  case DOT_LINK: {
    char src[DAMGR_PATH_BUFFER_MAX];
    char dst[DAMGR_PATH_BUFFER_MAX];
    DAMGR_CALL(damgr_snprintf_result(src, sizeof(src), "%s/%s",
                                     paths->active_dotfile_dir, name));
    DAMGR_CALL(damgr_snprintf_result(dst, sizeof(dst), "%s/.config/%s",
                                     paths->home, name));
    bool exists;
    bool is_link;
    DAMGR_CALL(is_link_result(dst, src, &exists, &is_link));
    if (exists && !is_link)
      return Error("%s exists and is not a symlink to %s", dst, src);
    if (cmd->type == DOT_LINK) {
      if (!exists && symlink(src, dst) != 0)
        return Error("could not link %s -> %s: %s", dst, src, strerror(errno));
      return Success;
    }
    if (is_link && unlink(dst) != 0)
      return Error("could not unlink %s: %s", dst, strerror(errno));
    return Success;
  }
  case AUR_REMOVE:
  case PKG_REMOVE:
    // return exec_result((char *[]){"sudo", "pacman", "-Rns", name, nullptr});
    return exec_result((char *[]){"sudo", "pacman", "-Rn", name, nullptr});
  case PKG_INSTALL:
    return exec_result(
        (char *[]){"sudo", "pacman", "-S", "--needed", name, nullptr});
  case AUR_INSTALL:
    return exec_result((char *[]){(char *)context->aur_helper, "-S", "--needed",
                                  name, nullptr});
  }
  return Error("not a valid task type");
}

Result_t damgr_merge_result(const Damgr_Paths *paths,
                            const Damgr_Context *context) {
  DAMGR_CALL(get_tasks_result(context));
  DAMGR_CALL(damgr_journal_write_result(paths)); // all PENDING

  for (size_t i = 0; i < damgr_journal.cnt; ++i) {
    Damgr_Cmd *cmd = &damgr_journal.cmds[i];
    auto r = run_cmd_result(paths, context, cmd);
    cmd->status = r.is_success ? SUCCEEDED : FAILED;
    DAMGR_CALL(damgr_journal_write_result(paths));
    if (!r.is_success) {
      return Error("%s %s failed, see %s", TYPE_STR[cmd->type], cmd->name,
                   paths->journal);
    }
  }

  return Success;
}

#endif /* DAMGR_MERGE_IMPLEMENTATION */
#endif /* DAMGR_MERGE_H */
