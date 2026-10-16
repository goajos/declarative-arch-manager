#ifndef DAMGR_STAGE_H
#define DAMGR_STAGE_H
#include "damgr/context.h"

int damgr_stage_with_context(const Damgr_Context *context);
int damgr_stage(Damgr_Arena *arena);

#endif /* DAMGR_STAGE_H */
