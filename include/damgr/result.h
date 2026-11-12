#ifndef DAMGR_RESULT_H
#define DAMGR_RESULT_H
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "damgr/arena.h"
#include "damgr/log.h"

constexpr size_t ERR_MSG_MAX = 256;

typedef struct err {
  char (*msgs)[ERR_MSG_MAX];
  size_t capacity;
  size_t cnt;
} Damgr_Err;

typedef struct Result {
  bool is_success;
} Result_t;

extern Damgr_Err damgr_err;

[[gnu::format(printf, 1, 2)]]
Result_t damgr_err_push(const char *fmt, ...);
void damgr_err_print();
void damgr_err_free();

#define Success ((Result_t){.is_success = true})
#define Error(...) damgr_err_push(__VA_ARGS__)
// the return will log the failing DAMGR_CALL
#define DAMGR_CALL(func)                                                       \
  do {                                                                         \
    if (!(func).is_success)                                                    \
      return damgr_err_push("%s() at %s:%d", __func__, __FILE__, __LINE__);    \
  } while (0)

Result_t damgr_arena_strdup_result(Damgr_Arena *arena, const char *src,
                                   char **out);
Result_t damgr_arena_alloc_result(Damgr_Arena *arena, size_t size, void **out);
Result_t damgr_snprintf_result(char *buffer, const size_t size, const char *fmt,
                               ...);
Result_t damgr_mkdir_result(const char *fid_buffer, const mode_t perm);

#ifdef DAMGR_RESULT_IMPLEMENTATION

Damgr_Err damgr_err = {0};

// Error() returns this, e.g. Result_t
Result_t damgr_err_push(const char *fmt, ...) {
  if (damgr_err.cnt >= damgr_err.capacity) {
    size_t capacity = damgr_err.capacity == 0 ? 2 : damgr_err.capacity * 2;
    char (*msgs)[ERR_MSG_MAX] =
        realloc(damgr_err.msgs, capacity * sizeof(*msgs));
    if (!msgs) {
      damgr_log(ERROR, "failed to realloc err msgs");
      return (Result_t){.is_success = false};
    }

    damgr_err.msgs = msgs;
    damgr_err.capacity = capacity;
  }

  va_list args;
  va_start(args, fmt);
  vsnprintf(damgr_err.msgs[damgr_err.cnt], ERR_MSG_MAX, fmt, args);
  va_end(args);

  damgr_err.cnt++;
  return (Result_t){.is_success = false};
}

void damgr_err_print() {
  for (size_t i = 0; i < damgr_err.cnt; ++i)
    damgr_log(ERROR, " #%zu %s", i, damgr_err.msgs[i]);
}

void damgr_err_free() {
  free(damgr_err.msgs);
  damgr_err = (Damgr_Err){0};
}

Result_t damgr_arena_strdup_result(Damgr_Arena *arena, const char *src,
                                   char **out) {
  *out = damgr_arena_strdup(arena, src);
  if (*out == nullptr)
    return Error("arena strdup failed");
  return Success;
}

Result_t damgr_arena_alloc_result(Damgr_Arena *arena, size_t size, void **out) {
  *out = damgr_arena_alloc(arena, size);
  if (*out == nullptr)
    return Error("arena alloc failed");
  return Success;
}

Result_t damgr_snprintf_result(char *buffer, const size_t size, const char *fmt,
                               ...) {
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buffer, size, fmt, args);
  va_end(args);
  if (n < 0 || (size_t)n >= size)
    return Error("\"%s\": %s", buffer, strerror(ENAMETOOLONG));
  return Success;
}

Result_t damgr_mkdir_result(const char *fid_buffer, const mode_t perm) {
  if (mkdir(fid_buffer, perm) != 0) {
    return Error("could not create dir \"%s\": %s", fid_buffer,
                 strerror(errno));
  }
  return Success;
}

#endif /* DAMGR_RESULT_IMPLEMENTATION */
#endif /* DAMGR_RESULT_H */
