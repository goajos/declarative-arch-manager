#ifndef RESULT_H
#define RESULT_H
#define _DEFAULT_SOURCE
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

#define Result_t(T, E)                                                         \
  struct Result_##T##_##E {                                                    \
    bool is_success;                                                           \
    union {                                                                    \
      T value;                                                                 \
      E error;                                                                 \
    };                                                                         \
  }

#define Success(T, E)                                                          \
  (struct Result_##T##_##E) {                                                  \
    .is_success = true, .value = (T)_SUCCESS_IMPL
#define _SUCCESS_IMPL(...)                                                     \
  __VA_ARGS__                                                                  \
  }

#define Err(T, E)                                                              \
  (struct Result_##T##_##E) {                                                  \
    .is_success = false, .error = (E)_ERR_IMPL
#define _ERR_IMPL(...)                                                         \
  __VA_ARGS__                                                                  \
  }

const char *err_fmt(const char *fmt, ...);

typedef void *Nullptr_t;
typedef void *Ptr_t;
typedef FILE *Fid_t;
typedef bool *Bool_t;
typedef const char *ErrMessage_t;

Result_t(Ptr_t, ErrMessage_t);

#ifdef DAMGR_RESULT_IMPLEMENTATION

const char *err_fmt(const char *fmt, ...) {
  static char err_buffer[DAMGR_RESULT_BUFFER_MAX];
  va_list args;
  va_start(args, fmt);
  vsnprintf(err_buffer, sizeof(err_buffer), fmt, args);
  va_end(args);
  return err_buffer;
}

Result_t(Ptr_t, ErrMessage_t)
    arena_strdup_r(Damgr_Arena *arena, const char *src) {
  Ptr_t dst = damgr_arena_strdup(arena, src);
  if (dst == nullptr)
    return Err(Ptr_t, ErrMessage_t)("arena strdup failed");
  return Success(Ptr_t, ErrMessage_t)(dst);
}

Result_t(Fid_t, ErrMessage_t)
    fopen_r(const char *fid_buffer, const char *mode) {
  Fid_t fid = fopen(fid_buffer, mode);
  if (fid == nullptr)
    return Err(Fid_t, ErrMessage_t)(
        err_fmt("could not open fid %s: %s", fid_buffer, strerror(errno)));
  return Success(Fid_t, ErrMessage_t)(fid);
}

Result_t(Nullptr_t, ErrMessage_t)
    snprintf_r(char *out, const size_t out_size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(out, out_size, fmt, args);
  va_end(args);
  if (n < 0 || (size_t)n >= out_size)
    return Err(Nullptr_t, ErrMessage_t)(
        err_fmt("\"%s\": %s", out, strerror(ENAMETOOLONG)));
  return Success(Nullptr_t, ErrMessage_t)(nullptr);
}

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

Result_t(Bool_t, ErrMessage_t)
    mkdir_r(const char *fid_buffer, const mode_t perm) {
  if (mkdir(fid_buffer, perm) != 0)
    return Err(Bool_t, ErrMessage_t)(err_fmt("could not create dir \"%s\": %s",
                                             fid_buffer, strerror(errno)));

  return Success(Bool_t, ErrMessage_t)(true);
}

#endif /* DAMGR_RESULT_IMPLEMENTATION */
#endif /* RESULT_H */
