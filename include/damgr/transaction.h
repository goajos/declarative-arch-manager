#ifndef DAMGR_TRANSACTION_H
#define DAMGR_TRANSACTION_H
#include "damgr/context.h"
#include "damgr/journal.h"
#include "damgr/log.h"
#include "damgr/paths.h"
#include "damgr/task.h"
#include "damgr/utils.h"
#include <unistd.h>

int damgr_transaction_preflight(const Damgr_Paths *paths,
                                const Damgr_Context *context,
                                const Damgr_Task_Queue *queu);

int damgr_transaction(const Damgr_Paths *paths, const char *aur_helper,
                      Damgr_Task_Queue *queue);

#endif /* DAMGR_TRANSACTION_H */
