#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include "damgr/log.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DAMGR_FID_BUF_MAX 4096
#define DAMGR_DIR_PERM 0755
#define DAMGR_BYTES_BUF_MAX 1024
#define DAMGR_JOURNAL ".journal"
#define DAMGR_STATE_FILE_PREFIX "state_"

void damgr_trim_string_inplace(char *str);

int damgr_copy_file(const char *src_buf, const char *dst_buf);

bool damgr_make_dir(const char *fid_buf);
bool damgr_dir_exists(const char *fid_buf);
bool damgr_is_dir_empty(const char *fid_buf);
int damgr_copy_dir(const char *src_buf, const char *dst_buf, const bool rename);
int damgr_copy_dir_except(const char *src_buf, const char *dst_buf,
                          const bool rename, const char *keep);
int damgr_clean_dir(const char *fid_buf, const bool state);
int damgr_clean_dir_except(const char *fid_buf, const bool state,
                           const char *keep);

#endif /* DAMGR_UTILS_H */
