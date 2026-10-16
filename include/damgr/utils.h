#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include "damgr/context.h"
#include "damgr/log.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define DAMGR_FID_BUF_MAX 4096
#define DAMGR_DIR_PERM 0755
#define DAMGR_BYTES_BUF_MAX 1024

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

void damgr_format_fid_buf(char *fid_buf, const char *name, const bool is_state,
                          const Damgr_Active_Dir_Type active_dir_type,
                          const Damgr_State_Dir_Type state_dir_type);

void damgr_trim_string_inplace(char *str);

int damgr_make_dir(const char *fid_buf);
// bool damgr_is_dir(const char *fid_buf);
// bool damgr_is_file(const char *fid_buf);
// bool damgr_is_dotfile(const char *fid_buf);

#endif /* DAMGR_UTILS_H */
