#ifndef DAMGR_RESULT_H
#define DAMGR_RESULT_H
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "damgr/arena.h"
#include "damgr/log.h"

constexpr size_t RESULT_BUFFER_MAX = 512;

typedef struct Result {
  bool is_success;
  const char *error;
} Result_t;

#define Success                                                                \
  (Result_t) { .is_success = true, .error = nullptr }
#define Error(err)                                                             \
  (Result_t) { .is_success = false, .error = err }
#define DAMGR_CALL(func)                                                       \
  do {                                                                         \
    Result_t _r = (func);                                                      \
    if (!_r.is_success)                                                        \
      return _r;                                                               \
  } while (0)

const char *damgr_err_fmt(const char *fmt, ...);
Result_t damgr_arena_strdup_r(Damgr_Arena *arena, const char *src, char **out);
Result_t damgr_arena_alloc_r(Damgr_Arena *arena, size_t size, void **out);
Result_t damgr_fopen_r(const char *fid_buffer, const char *mode, FILE **out);
Result_t damgr_opendir_r(const char *fid_buffer, DIR **out);
Result_t damgr_snprintf_r(char *buffer, const size_t size, const char *fmt,
                          ...);
Result_t damgr_mkdir_r(const char *fid_buffer, const mode_t perm);

#ifdef DAMGR_RESULT_IMPLEMENTATION

const char *damgr_err_fmt(const char *fmt, ...) {
  static char err_buffer[RESULT_BUFFER_MAX];
  va_list args;
  va_start(args, fmt);
  vsnprintf(err_buffer, sizeof(err_buffer), fmt, args);
  va_end(args);
  return err_buffer;
}

Result_t damgr_arena_strdup_r(Damgr_Arena *arena, const char *src, char **out) {
  *out = damgr_arena_strdup(arena, src);
  if (*out == nullptr)
    return Error("arena strdup failed");
  return Success;
}

Result_t damgr_arena_alloc_r(Damgr_Arena *arena, size_t size, void **out) {
  *out = damgr_arena_alloc(arena, size);
  if (*out == nullptr)
    return Error("arena alloc failed");
  return Success;
}

Result_t damgr_fopen_r(const char *fid_buffer, const char *mode, FILE **out) {
  *out = fopen(fid_buffer, mode);
  if (*out == nullptr)
    return Error(damgr_err_fmt("could not open fid %s: %s", fid_buffer,
                               strerror(errno)));
  return Success;
}

Result_t damgr_opendir_r(const char *fid_buffer, DIR **out) {
  *out = opendir(fid_buffer);
  if (*out == nullptr)
    return Error(damgr_err_fmt("could not open dir %s: %s", fid_buffer,
                               strerror(errno)));
  return Success;
}

Result_t damgr_snprintf_r(char *buffer, const size_t size, const char *fmt,
                          ...) {
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buffer, size, fmt, args);
  va_end(args);
  if (n < 0 || (size_t)n >= size)
    return Error(damgr_err_fmt("\"%s\": %s", buffer, strerror(ENAMETOOLONG)));
  return Success;
}

Result_t damgr_mkdir_r(const char *fid_buffer, const mode_t perm) {
  if (mkdir(fid_buffer, perm) != 0) {
    return Error(damgr_err_fmt("could not create dir \"%s\": %s", fid_buffer,
                               strerror(errno)));
  }
  return Success;
}

#endif /* DAMGR_RESULT_IMPLEMENTATION */
#endif /* DAMGR_RESULT_H */
