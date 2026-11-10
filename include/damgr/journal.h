#ifndef DAMGR_JOURNAL_H
#define DAMGR_JOURNAL_H
#include <ctype.h>
#include <string.h>

#include "damgr/context.h"
#include "damgr/paths.h"
#include "damgr/result.h"

/* journal format, one line per cmd:
 *   # damgr journal: ...
 *   SUCCEEDED	nvim	PKG_INSTALL	nvim
 *   SUCCEEDED	nvim	PKG_INSTALL	tree-sitter-cli
 *   FAILED	nvim	SVC_ENABLE	foo.service:true
 *   PENDING	tmux	PKG_INSTALL	tmux
 *   DONE
 *
 * merge appends the DONE line once every cmd SUCCEEDED. on failure the user
 * manually performs the FAILED/PENDING cmds and adds DONE.
 */

#define JOURNAL_DONE "DONE"

// declaration order == execution order
typedef enum task_type {
  SVC_DISABLE,
  DOT_UNLINK,
  AUR_REMOVE,
  PKG_REMOVE,
  PKG_INSTALL,
  AUR_INSTALL,
  DOT_LINK,
  SVC_ENABLE,
} Damgr_Task_Type;

typedef enum status { PENDING, SUCCEEDED, FAILED } Damgr_Status;

typedef struct cmd {
  Damgr_Task_Type type;
  const char *mod_name;
  const char *name; // package, service or dotfile name
  bool flag;        // services only: true = system (sudo), false = --user
  Damgr_Status status;
} Damgr_Cmd;

typedef struct journal {
  Damgr_Cmd *cmds;
  size_t cnt;
  size_t capacity;
} Damgr_Journal;

const char *damgr_task_type_string(Damgr_Task_Type type);
const char *damgr_status_string(Damgr_Status status);

Result_t damgr_journal_push_r(Damgr_Journal *journal, Damgr_Task_Type type,
                              const char *mod_name, const char *name,
                              bool flag);
void damgr_journal_free(Damgr_Journal *journal);

Result_t damgr_journal_write_r(const Damgr_Paths *paths,
                               const Damgr_Journal *journal);
Result_t damgr_journal_is_done_r(const Damgr_Paths *paths, bool *done);
Result_t damgr_journal_delete_r(const Damgr_Paths *paths);

#ifdef DAMGR_JOURNAL_IMPLEMENTATION

const char *damgr_task_type_string(Damgr_Task_Type type) {
  switch (type) {
  case SVC_DISABLE:
    return "SVC_DISABLE";
  case DOT_UNLINK:
    return "DOT_UNLINK";
  case AUR_REMOVE:
    return "AUR_REMOVE";
  case PKG_REMOVE:
    return "PKG_REMOVE";
  case PKG_INSTALL:
    return "PKG_INSTALL";
  case AUR_INSTALL:
    return "AUR_INSTALL";
  case DOT_LINK:
    return "DOT_LINK";
  case SVC_ENABLE:
    return "SVC_ENABLE";
  }
  return "?";
}

const char *damgr_status_string(Damgr_Status status) {
  switch (status) {
  case PENDING:
    return "PENDING";
  case SUCCEEDED:
    return "SUCCEEDED";
  case FAILED:
    return "FAILED";
  }
  return "?";
}

Result_t damgr_journal_push_r(Damgr_Journal *journal, Damgr_Task_Type type,
                              const char *mod_name, const char *name,
                              bool flag) {
  if (journal->cnt >= journal->capacity) {
    size_t capacity = journal->capacity == 0 ? 32 : journal->capacity * 2;
    Damgr_Cmd *cmds = realloc(journal->cmds, capacity * sizeof(Damgr_Cmd));
    if (cmds == nullptr)
      return Error("journal realloc failed");
    journal->cmds = cmds;
    journal->capacity = capacity;
  }

  journal->cmds[journal->cnt++] = (Damgr_Cmd){.type = type,
                                              .mod_name = mod_name,
                                              .name = name,
                                              .flag = flag,
                                              .status = PENDING};
  return Success;
}

void damgr_journal_free(Damgr_Journal *journal) {
  free(journal->cmds);
  *journal = (Damgr_Journal){0};
}

static Result_t write_journal_fid_r(FILE *fid, const Damgr_Journal *journal) {
  fprintf(fid,
          "# damgr journal: on failure rollback or manually perform the "
          "FAILED/PENDING cmds, then add a line " JOURNAL_DONE " to commit\n");

  bool done = true;
  for (size_t i = 0; i < journal->cnt; ++i) {
    const Damgr_Cmd *cmd = &journal->cmds[i];
    if (cmd->status != SUCCEEDED)
      done = false;

    fprintf(fid, "%s\t%s\t%s\t%s", damgr_status_string(cmd->status),
            cmd->mod_name, damgr_task_type_string(cmd->type), cmd->name);
    if (cmd->type == SVC_ENABLE || cmd->type == SVC_DISABLE)
      fprintf(fid, ":%s", cmd->flag ? "true" : "false");
    fprintf(fid, "\n");
  }
  if (done) // an empty journal is done
    fprintf(fid, JOURNAL_DONE "\n");

  if (ferror(fid))
    return Error("could not write journal");
  if (fflush(fid) != 0)
    return Error(damgr_err_fmt("could not flush journal: %s", strerror(errno)));
  return Success;
}

// atomic: write <journal>.new, then rename over <journal>
Result_t damgr_journal_write_r(const Damgr_Paths *paths,
                               const Damgr_Journal *journal) {
  char tmp[PATH_BUFFER_MAX];
  DAMGR_CALL(damgr_snprintf_r(tmp, sizeof(tmp), "%s.new", paths->journal));

  FILE *fid;
  DAMGR_CALL(damgr_fopen_r(tmp, "w", &fid));
  // save result so we can ensure fid is closed and tmp unlinked on failure
  // before return
  Result_t r = write_journal_fid_r(fid, journal);
  fclose(fid);
  if (!r.is_success) {
    unlink(tmp);
    return r;
  }

  // save result so we can ensure tmp is unlinked on failure before return
  r = damgr_rename_r(tmp, paths->journal);
  if (!r.is_success) {
    unlink(tmp);
    return r;
  }
  return Success;
}

// done if the journal contains a non-indented DONE line
static Result_t scan_done_r(FILE *fid, bool *done) {
  char line[DAMGR_LINE_MAX];
  while (fgets(line, sizeof(line), fid)) {
    size_t len = strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1]))
      line[--len] = '\0';
    if (strcmp(line, JOURNAL_DONE) == 0) {
      *done = true;
      return Success;
    }
  }
  if (ferror(fid))
    return Error("could not read journal");
  return Success;
}

Result_t damgr_journal_is_done_r(const Damgr_Paths *paths, bool *done) {
  *done = false;
  FILE *fid;
  DAMGR_CALL(damgr_fopen_r(paths->journal, "r", &fid));
  // save result so we can ensure fid is closed before return
  Result_t r = scan_done_r(fid, done);
  fclose(fid);
  return r;
}

Result_t damgr_journal_delete_r(const Damgr_Paths *paths) {
  return damgr_unlink_r(paths->journal);
}

#endif /* DAMGR_JOURNAL_IMPLEMENTATION */
#endif /* DAMGR_JOURNAL_H */
