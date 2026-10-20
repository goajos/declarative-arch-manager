#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
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

#ifdef DAMGR_PATHS_IMPLEMENTATION

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

  auto result = lstat_r(p_buffer, st, S_ISDIR, ENOENT);
  if (!result.is_success) {
    return result;
  }
  if (result.value) // parent already exists
    return Success(Bool_t, ErrMessage_t)(true);

  result = damgr_make_dir_p(p_buffer, st);
  if (!result.is_success) {
    return result;
  }

  return mkdir_r(p_buffer, DAMGR_DIR_PERM);
}

Result_t(Bool_t, ErrMessage_t)
    damgr_make_dir(const char *fid_buffer, struct stat *st) {
  auto result = lstat_r(fid_buffer, st, S_ISDIR, ENOENT);
  if (!result.is_success) {
    return result;
  }

  result = damgr_make_dir_p(fid_buffer, st);
  if (!result.is_success) {
    return result;
  }

  return mkdir_r(fid_buffer, DAMGR_DIR_PERM);
}

#endif /* DAMGR_UTILS_IMPLEMENTATION */
#endif /* DAMGR_UTILS_H */
