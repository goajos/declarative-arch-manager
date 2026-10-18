#ifndef DAMGR_JOURNAL_H
#define DAMGR_JOURNAL_H
#include "damgr/context.h"
#include "damgr/task.h"
#include "damgr/utils.h"

#ifndef DAMGR_JOURNAL
#define DAMGR_JOURNAL ".journal"
#endif

typedef enum journal_phase {
  DAMGR_JOURNAL_RUNNING,
  DAMGR_JOURNAL_ROLLBACK,
  DAMGR_JOURNAL_DONE,
} Damgr_Journal_Phase;
const char *journal_phase_string(const Damgr_Journal_Phase phase);

int damgr_write_journal(const char *journal_buf,
                        const Damgr_Journal_Phase phase,
                        const Damgr_Task_Queue *queue);

int damgr_read_journal(const char *journal_buf, int *phase, int *task_cnt);

int damgr_clean_journal(const char *journal_buf);

#endif /* DAMGR_JOURNAL_H */
