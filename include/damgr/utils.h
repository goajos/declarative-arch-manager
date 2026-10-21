#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include <dirent.h>
#include <string.h>

#ifndef DAMGR_RESULT_H
#include "damgr/result.h"
#endif

#ifndef DAMGR_PATHS_H
#include "damgr/paths.h"
#endif

#define DAMGR_JOURNAL ".journal"
constexpr mode_t DAMGR_DIR_PERM = 0755;

Result_t(Bool_t, ErrMessage_t)
    damgr_make_dir(const char *fid_buffer, struct stat *st);

#define lstat_r(fid_buffer, st, mode)                                          \
  ({                                                                           \
    Result_t(Bool_t, ErrMessage_t) _bool_r;                                    \
    if (lstat(fid_buffer, st) == 0) {                                          \
      bool _matched = mode(st->st_mode);                                       \
      if (!_matched) {                                                         \
        _bool_r = Err(Bool_t, ErrMessage_t)(                                   \
            err_fmt("%s exists but is wrong file type: %s", fid_buffer,        \
                    strerror(errno)));                                         \
      } else {                                                                 \
        /* exists and is right file type, so true  */                          \
        _bool_r = Success(Bool_t, ErrMessage_t)(true);                         \
      }                                                                        \
    } else if (errno != ENOENT) {                                              \
      _bool_r = Err(Bool_t, ErrMessage_t)(                                     \
          err_fmt("lstat failed: %s", strerror(errno)));                       \
    } else {                                                                   \
      /* errno == ENOENT means the file doesn't exist, so false */             \
      _bool_r = Success(Bool_t, ErrMessage_t)(false);                          \
    }                                                                          \
    _bool_r;                                                                   \
  })

#ifdef DAMGR_UTILS_IMPLEMENTATION

static Result_t(Bool_t, ErrMessage_t)
    damgr_make_dir_p(const char *fid_buffer, struct stat *st) {
  const char *sep = strrchr(fid_buffer, '/');
  if (sep == nullptr || sep == fid_buffer)
    return Success(Bool_t, ErrMessage_t)(true);

  char p_buffer[DAMGR_PATH_BUFFER_MAX];
  size_t len = (size_t)(sep - fid_buffer);
  if (len >= sizeof(p_buffer))
    return Err(Bool_t, ErrMessage_t)(
        err_fmt("\"%s\": %s", p_buffer, strerror(ENAMETOOLONG)));

  memcpy(p_buffer, fid_buffer, len);
  p_buffer[len] = '\0';

  auto bool_r = lstat_r(p_buffer, st, S_ISDIR);
  if (!bool_r.is_success)
    return bool_r;

  if (bool_r.value) // parent already exists
    return Success(Bool_t, ErrMessage_t)(true);

  bool_r = damgr_make_dir_p(p_buffer, st);
  if (!bool_r.is_success)
    return bool_r;

  return mkdir_r(p_buffer, DAMGR_DIR_PERM);
}

Result_t(Bool_t, ErrMessage_t)
    damgr_make_dir(const char *fid_buffer, struct stat *st) {
  auto bool_r = lstat_r(fid_buffer, st, S_ISDIR);
  if (!bool_r.is_success)
    return bool_r;

  if (bool_r.value) // folder already exists
    return Success(Bool_t, ErrMessage_t)(true);

  bool_r = damgr_make_dir_p(fid_buffer, st);
  if (!bool_r.is_success) {
    return bool_r;
  }

  return mkdir_r(fid_buffer, DAMGR_DIR_PERM);
}

Result_t(Bool_t, ErrMessage_t) damgr_is_dir_empty(const char *fid_buffer) {
  auto dir_r = opendir_r(fid_buffer);
  if (!dir_r.is_success)
    return Err(Bool_t, ErrMessage_t)(dir_r.error);

  DIR *dir = dir_r.value;
  int count = 0;
  struct dirent *ent;
  // .backup and .staging are not .state/damgr entries
  while ((ent = readdir(dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 ||
        strcmp(ent->d_name, ".backup") == 0 ||
        strcmp(ent->d_name, ".staging") == 0)
      continue;

    if (strcmp(ent->d_name, DAMGR_JOURNAL) == 0)
      continue; // the journal is not a .state/damgr entry

    if (ent->d_type == DT_REG || ent->d_type == DT_LNK || ent->d_type == DT_DIR)
      count++;
  }

  closedir(dir);
  if (count == 0) {
    return Success(Bool_t, ErrMessage_t)(true);
  } else {
    return Err(Bool_t, ErrMessage_t)(
        err_fmt("%s: %s", fid_buffer, strerror(ENOTEMPTY)));
  }
}

#endif /* DAMGR_UTILS_IMPLEMENTATION */
#endif /* DAMGR_UTILS_H */
