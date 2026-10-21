#ifndef RESULT_H
#define RESULT_H
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifndef DAMGR_ARENA_H
#include "damgr/arena.h"
#endif

#ifndef DAMGR_LOG_H
#include "damgr/log.h"
#endif

constexpr size_t DAMGR_RESULT_BUFFER_MAX = 512;

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
    Result_t _s = (func);                                                      \
    if (!_s.is_success)                                                        \
      return _s;                                                               \
  } while (0)

const char *err_fmt(const char *fmt, ...);

#ifdef DAMGR_RESULT_IMPLEMENTATION

const char *err_fmt(const char *fmt, ...) {
  static char err_buffer[DAMGR_RESULT_BUFFER_MAX];
  va_list args;
  va_start(args, fmt);
  vsnprintf(err_buffer, sizeof(err_buffer), fmt, args);
  va_end(args);
  return err_buffer;
}

Result_t arena_strdup_r(Damgr_Arena *arena, const char *src, char **out) {
  *out = arena_strdup(arena, src);
  if (*out == nullptr)
    return Error("arena strdup failed");
  return Success;
}

Result_t arena_alloc_r(Damgr_Arena *arena, size_t size, void **out) {
  *out = arena_alloc(arena, size);
  if (*out == nullptr)
    return Error("arena alloc failed");
  return Success;
}

Result_t fopen_r(const char *fid_buffer, const char *mode, FILE **out) {
  *out = fopen(fid_buffer, mode);
  if (*out == nullptr)
    return Error(
        err_fmt("could not open fid %s: %s", fid_buffer, strerror(errno)));
  return Success;
}

Result_t opendir_r(const char *fid_buffer, DIR **out) {
  *out = opendir(fid_buffer);
  if (*out == nullptr)
    return Error(
        err_fmt("could not open dir %s: %s", fid_buffer, strerror(errno)));
  return Success;
}

Result_t snprintf_r(char *buffer, const size_t size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buffer, size, fmt, args);
  va_end(args);
  if (n < 0 || (size_t)n >= size)
    return Error(err_fmt("\"%s\": %s", buffer, strerror(ENAMETOOLONG)));
  return Success;
}

Result_t mkdir_r(const char *fid_buffer, const mode_t perm) {
  if (mkdir(fid_buffer, perm) != 0) {
    return Error(err_fmt("could not create dir \"%s\": %s", fid_buffer,
                         strerror(errno)));
  }
  return Success;
}

#endif /* DAMGR_RESULT_IMPLEMENTATION */
#endif /* RESULT_H */
