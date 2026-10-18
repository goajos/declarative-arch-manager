#ifndef DAMGR_TRANSACTION_H
#define DAMGR_TRANSACTION_H
#include "damgr/context.h"
#include "damgr/journal.h"
#include "damgr/log.h"
#include "damgr/task.h"
#include "damgr/utils.h"
#include <unistd.h>

int damgr_transaction_preflight(const Damgr_Context *context,
                                const Damgr_Task_Queue *queue);

int damgr_transaction(const char *aur_helper, const char *journal_buf,
                      Damgr_Task_Queue *queue);

#endif /* DAMGR_TRANSACTION_H */
