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

constexpr mode_t DAMGR_DIR_PERM = 0755;

Result_t(Bool_t, ErrMessage_t)
    damgr_make_dir(const char *fid_buffer, struct stat *st);

#define lstat_r(fid_buffer, st, mode)                                          \
  ({                                                                           \
    Result_t(Bool_t, ErrMessage_t) _result;                                    \
    if (lstat(fid_buffer, st) == 0) {                                          \
      bool _matched = mode(st->st_mode);                                       \
      if (!_matched) {                                                         \
        _result = Err(Bool_t, ErrMessage_t)(                                   \
            err_fmt("%s exists but is wrong file type: %s", fid_buffer,        \
                    strerror(errno)));                                         \
      } else {                                                                 \
        /* exists and is right file type, so true  */                          \
        _result = Success(Bool_t, ErrMessage_t)(true);                         \
      }                                                                        \
    } else if (errno != ENOENT) {                                              \
      _result = Err(Bool_t, ErrMessage_t)(                                     \
          err_fmt("lstat failed: %s", strerror(errno)));                       \
    } else {                                                                   \
      /* errno == ENOENT means the file doesn't exist, so false */             \
      _result = Success(Bool_t, ErrMessage_t)(false);                          \
    }                                                                          \
    _result;                                                                   \
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

  auto result = lstat_r(p_buffer, st, S_ISDIR);
  if (!result.is_success)
    return result;

  if (result.value) // parent already exists
    return Success(Bool_t, ErrMessage_t)(true);

  result = damgr_make_dir_p(p_buffer, st);
  if (!result.is_success)
    return result;

  return mkdir_r(p_buffer, DAMGR_DIR_PERM);
}

Result_t(Bool_t, ErrMessage_t)
    damgr_make_dir(const char *fid_buffer, struct stat *st) {
  auto result = lstat_r(fid_buffer, st, S_ISDIR);
  if (!result.is_success)
    return result;

  if (result.value) // folder already exists
    return Success(Bool_t, ErrMessage_t)(true);

  result = damgr_make_dir_p(fid_buffer, st);
  if (!result.is_success) {
    return result;
  }

  return mkdir_r(fid_buffer, DAMGR_DIR_PERM);
}

Result_t(Bool_t, ErrMessage_t) damgr_is_dir_empty(const char *fid_buffer) {
  auto result = opendir_r(fid_buffer);
  if (!result.is_success)
    return Err(Bool_t, ErrMessage_t)(result.error);

  DIR *dir = result.value;
  int count = 0;
  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;

    // TODO: do we want to check for dirs here?
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
