#ifndef DAMGR_STAGE_H
#define DAMGR_STAGE_H
#include "damgr/context.h"
#include "damgr/paths.h"

int damgr_stage_with_context(const Damgr_Context *context,
                             const Damgr_Paths *paths);
int damgr_stage(const Damgr_Paths *paths, Damgr_Arena *arena);

#endif /* DAMGR_STAGE_H */
