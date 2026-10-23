#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include <dirent.h>
#include <string.h>

#include "damgr/paths.h"
#include "damgr/result.h"

#define TMP ".tmp"
#define JOURNAL ".journal"
constexpr mode_t DIR_PERM = 0755;

typedef enum filetype { DIR_TYPE, REG_TYPE } Damgr_Filetype;

Result_t damgr_make_dir_r(const char *fid_buffer, struct stat *st, bool *out);
Result_t damgr_is_dir_empty_r(const char *fid_buffer, bool *out);
Result_t damgr_dir_exists_r(const char *fid_buffer, struct stat *st, bool *out);

#ifdef DAMGR_UTILS_IMPLEMENTATION

static Result_t lstat_r(const char *fid_buffer, struct stat *st,
                        Damgr_Filetype type, bool *out) {
  if (lstat(fid_buffer, st) == 0) {
    bool matched =
        (type == DIR_TYPE) ? S_ISDIR(st->st_mode) : S_ISREG(st->st_mode);

    if (!matched)
      return Error(
          damgr_err_fmt("%s exists but is wrong file type", fid_buffer));
    // exists and is right file type, so true
    *out = true;
    return Success;
  }
  if (errno != ENOENT)
    return Error(damgr_err_fmt("lstat failed: %s", strerror(errno)));
  // errno == ENOENT means the file doesn't exist, so false
  *out = false;
  return Success;
}

static Result_t make_dir_parent_r(const char *fid_buffer, struct stat *st,
                                  bool *out) {
  const char *sep = strrchr(fid_buffer, '/');
  if (sep == nullptr || sep == fid_buffer) {
    return Success;
  }

  char p_buffer[PATH_BUFFER_MAX];
  size_t len = (size_t)(sep - fid_buffer);
  if (len >= sizeof(p_buffer))
    return Error(
        damgr_err_fmt("\"%s\": %s", fid_buffer, strerror(ENAMETOOLONG)));

  memcpy(p_buffer, fid_buffer, len);
  p_buffer[len] = '\0';

  DAMGR_CALL(lstat_r(p_buffer, st, DIR_TYPE, out));

  if (*out) // parent already exists
    return Success;

  DAMGR_CALL(make_dir_parent_r(p_buffer, st, out));

  return damgr_mkdir_r(p_buffer, DIR_PERM);
}

Result_t damgr_make_dir_r(const char *fid_buffer, struct stat *st, bool *out) {
  DAMGR_CALL(lstat_r(fid_buffer, st, DIR_TYPE, out));

  if (*out) // folder already exists
    return Success;

  DAMGR_CALL(make_dir_parent_r(fid_buffer, st, out));

  return damgr_mkdir_r(fid_buffer, DIR_PERM);
}

Result_t damgr_is_dir_empty_r(const char *fid_buffer, bool *out) {
  DIR *dir;
  DAMGR_CALL(damgr_opendir_r(fid_buffer, &dir));

  int count = 0;
  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;

    if (ent->d_type == DT_REG || ent->d_type == DT_LNK ||
        ent->d_type == DT_DIR || ent->d_type == DT_UNKNOWN)
      count++;
  }

  closedir(dir);
  if (count == 0) {
    *out = true;
    return Success;
  } else {
    *out = false;
    return Success;
  }
}

Result_t damgr_dir_exists_r(const char *fid_buffer, struct stat *st,
                            bool *out) {
  DAMGR_CALL(lstat_r(fid_buffer, st, DIR_TYPE, out));
  return Success;
}

#endif /* DAMGR_UTILS_IMPLEMENTATION */
#endif /* DAMGR_UTILS_H */
