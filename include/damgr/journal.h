#ifndef DAMGR_JOURNAL_H
#define DAMGR_JOURNAL_H
#include <ctype.h>
#include <string.h>

#include "damgr/path.h"
#include "damgr/result.h"

typedef enum task_type {
  SVC_DISABLE,
  DOT_UNLINK,
  AUR_REMOVE,
  PKG_REMOVE, // this idx determines if a task is negative
  PKG_INSTALL,
  AUR_INSTALL,
  DOT_LINK,
  SVC_ENABLE,
} Damgr_Task_Type;

typedef enum status { PENDING, SUCCEEDED, FAILED } Damgr_Status;

typedef struct cmd {
  Damgr_Task_Type type;
  const char *mod_name;
  const char *name;
  bool flag;
  Damgr_Status status;
} Damgr_Cmd;

typedef struct journal {
  Damgr_Cmd *cmds;
  size_t capacity;
  size_t cnt;
} Damgr_Journal;

extern Damgr_Journal damgr_journal;

Result_t damgr_journal_push_result(const Damgr_Task_Type type,
                                   const char *mod_name, const char *out_name,
                                   bool flag);

Result_t damgr_journal_write_result(const Damgr_Paths *paths);

void damgr_journal_free();
/*
 */

#ifdef DAMGR_JOURNAL_IMPLEMENTATION

Damgr_Journal damgr_journal = {0};

static const char *const TYPE_STR[] = {
    [SVC_DISABLE] = "SVC_DISABLE", [DOT_UNLINK] = "DOT_UNLINK",
    [AUR_REMOVE] = "AUR_REMOVE",   [PKG_REMOVE] = "PKG_REMOVE",
    [PKG_INSTALL] = "PKG_INSTALL", [AUR_INSTALL] = "AUR_INSTALL",
    [DOT_LINK] = "DOT_LINK",       [SVC_ENABLE] = "SVC_ENABLE"};
static const char *const STATUS_STR[] = {
    [PENDING] = "PENDING", [SUCCEEDED] = "SUCCEEDED", [FAILED] = "FAILED"};

Result_t damgr_journal_push_result(const Damgr_Task_Type type,
                                   const char *mod_name, const char *out_name,
                                   bool flag) {
  if (damgr_journal.cnt >= damgr_journal.capacity) {
    size_t capacity =
        damgr_journal.capacity == 0 ? 2 : damgr_journal.capacity * 2;
    Damgr_Cmd *cmds = realloc(damgr_journal.cmds, capacity * sizeof(*cmds));
    if (!cmds) {
      return Error("failed to realloc journal cmds");
    }

    damgr_journal.cmds = cmds;
    damgr_journal.capacity = capacity;
  }

  damgr_journal.cmds[damgr_journal.cnt++] = (Damgr_Cmd){.type = type,
                                                        .mod_name = mod_name,
                                                        .name = out_name,
                                                        .flag = flag,
                                                        .status = PENDING};

  return Success;
}

static Result_t write_lines(FILE *fid, const Damgr_Paths *paths) {
  bool done = true;
  for (size_t i = 0; i < damgr_journal.cnt; ++i) {
    const Damgr_Cmd *cmd = &damgr_journal.cmds[i];
    const Damgr_Task_Type type = cmd->type;
    done = done && cmd->status == SUCCEEDED;
    if (type == SVC_DISABLE || type == SVC_ENABLE) {
      fprintf(fid, "%s\t%s\t%s\t%s:%s\n", STATUS_STR[cmd->status],
              TYPE_STR[cmd->type], cmd->mod_name, cmd->name,
              cmd->flag ? "true" : "false");
    } else {
      fprintf(fid, "%s\t%s\t%s\t%s\n", STATUS_STR[cmd->status],
              TYPE_STR[cmd->type], cmd->mod_name, cmd->name);
    }
  }
  if (done)
    fprintf(fid, "DONE\n");

  fflush(fid); // write the buffer to file
  if (ferror(fid))
    return Error("could not write %s", paths->journal);

  return Success;
}

Result_t damgr_journal_write_result(const Damgr_Paths *paths) {
  FILE *fid;
  DAMGR_CALL(damgr_fopen_result(paths->journal, "w", &fid));
  auto r = write_lines(fid, paths);
  fclose(fid);
  return r;
}

void damgr_journal_free() {
  free(damgr_journal.cmds);
  damgr_journal = (Damgr_Journal){0};
}

#endif /* DAMGR_JOURNAL_IMPLEMENTATION */
#endif /* DAMGR_JOURNAL_H */
