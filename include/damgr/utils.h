#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include "damgr/context.h"
#include <ctype.h>
#include <stdio.h>

#ifndef DAMGR_FIDBUF_MAX
#define DAMGR_FIDBUF_MAX 4096
#endif

typedef enum active_dir_type {
  CONTEXT,
  HOST,
  MODULE,
  DOTFILE,
  NO_ACTIVE_DIR
} Damgr_Active_Dir_Type;

typedef enum state_dir_type {
  STATE,
  STAGING,
  BACKUP,
  NO_STATE_DIR
} Damgr_State_Dir_Type;

void damgr_format_fidbuf(char *fidbuf, const char *name, const bool is_state,
                         const Damgr_Active_Dir_Type active_dir_type,
                         const Damgr_State_Dir_Type state_dir_type);
void damgr_trim_string_inplace(char *str);

#endif /* DAMGR_UTILS_H */
