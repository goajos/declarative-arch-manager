#ifndef DAMGR_COMMIT_H
#define DAMGR_COMMIT_H
#include "damgr/context.h"

int damgr_commit_with_context(const Damgr_Context context);
int damgr_commit(Damgr_Arena *arena);

#endif /* DAMGR_COMMIT_H */
