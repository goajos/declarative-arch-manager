#ifndef DAMGR_MERGE_H
#define DAMGR_MERGE_H
#include "damgr/arena.h"
#include "damgr/commit.h"
#include "damgr/diff.h"
#include "damgr/journal.h"
#include "damgr/stage.h"
#include "damgr/task.h"
#include "damgr/transaction.h"
#include "damgr/utils.h"

int damgr_merge(Damgr_Arena *arena);

#endif /* DAMGR_MERGE_H */
