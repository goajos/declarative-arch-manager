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

void damgr_journal_free();
/*
 */

#ifdef DAMGR_JOURNAL_IMPLEMENTATION

Damgr_Journal damgr_journal = {0};

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

void damgr_journal_free() {
  free(damgr_journal.cmds);
  damgr_journal = (Damgr_Journal){0};
}

#endif /* DAMGR_JOURNAL_IMPLEMENTATION */
#endif /* DAMGR_JOURNAL_H */
