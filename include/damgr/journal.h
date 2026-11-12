#ifndef DAMGR_JOURNAL_H
#define DAMGR_JOURNAL_H
#include <ctype.h>
#include <string.h>

#include "damgr/path.h"
#include "damgr/result.h"

typedef struct journal {
  char *cmds;
  size_t capacity;
  size_t cnt;
} Damgr_Journal;

extern Damgr_Journal damgr_journal;

Result_t damgr_journal_push();
void damgr_journal_free();

/*
 */

#ifdef DAMGR_JOURNAL_IMPLEMENTATION

Damgr_Journal damgr_journal = {0};

Result_t damgr_journal_push() {
  if (damgr_journal.cnt >= damgr_journal.capacity) {
    size_t capacity =
        damgr_journal.capacity == 0 ? 2 : damgr_journal.capacity * 2;
    char *cmds = realloc(damgr_journal.cmds, capacity * sizeof(*cmds));
    if (!cmds) {
      return Error("failed to realloc journal cmds");
    }

    damgr_journal.cmds = cmds;
    damgr_journal.capacity = capacity;
  }

  return Success;
}

void damgr_journal_free() {
  free(damgr_journal.cmds);
  damgr_journal = (Damgr_Journal){0};
}

#endif /* DAMGR_JOURNAL_IMPLEMENTATION */
#endif /* DAMGR_JOURNAL_H */
