#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include <dirent.h>
#include <string.h>

#include "damgr/arena.h"
#include "damgr/result.h"

#define TMP ".tmp"
#define JOURNAL ".journal"
constexpr mode_t DIR_PERM = 0755;
constexpr size_t COPY_BUFFER_MAX = 4096;
constexpr size_t PATH_BUFFER_MAX = 4096;

typedef enum filetype { DIR_TYPE, REG_TYPE } Damgr_Filetype;

Result_t damgr_arena_strdup_result(Damgr_Arena *arena, const char *src,
                                   char **out);
Result_t damgr_arena_alloc_result(Damgr_Arena *arena, size_t size, void **out);
Result_t damgr_snprintf_result(char *buffer, const size_t size, const char *fmt,
                               ...);
Result_t damgr_mkdir_result(const char *fid_buffer, const mode_t perm);

Result_t damgr_fopen_result(const char *fid_buffer, const char *mode,
                            FILE **out);
Result_t damgr_opendir_result(const char *fid_buffer, DIR **out);

Result_t damgr_make_dir_result(const char *fid_buffer, bool *exists);
Result_t damgr_dir_exists_result(const char *fid_buffer, bool *out);
Result_t damgr_reg_exists_result(const char *fid_buffer, bool *out);

#ifdef DAMGR_UTILS_IMPLEMENTATION

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

Result_t damgr_fopen_result(const char *fid_buffer, const char *mode,
                            FILE **out) {
  *out = fopen(fid_buffer, mode);
  if (*out == nullptr)
    return Error("could not open fid %s: %s", fid_buffer, strerror(errno));
  return Success;
}

Result_t damgr_opendir_result(const char *fid_buffer, DIR **out) {
  *out = opendir(fid_buffer);
  if (*out == nullptr)
    return Error("could not open dir %s: %s", fid_buffer, strerror(errno));
  return Success;
}

static Result_t lstat_result(const char *fid_buffer, Damgr_Filetype type,
                             bool *out) {
  struct stat st;
  if (lstat(fid_buffer, &st) == 0) {
    bool matched =
        (type == DIR_TYPE) ? S_ISDIR(st.st_mode) : S_ISREG(st.st_mode);

    if (!matched)
      return Error("%s exists but is wrong file type", fid_buffer);

    // exists and is right file type, so true
    *out = true;
    return Success;
  }
  if (errno != ENOENT)
    return Error("lstat %s failed: %s", fid_buffer, strerror(errno));
  // errno == ENOENT means the file doesn't exist, so false
  *out = false;
  return Success;
}

static Result_t make_dir_parent_result(const char *fid_buffer) {
  const char *sep = strrchr(fid_buffer, '/');
  if (sep == nullptr || sep == fid_buffer) {
    return Success;
  }

  char p_buffer[PATH_BUFFER_MAX];
  size_t len = (size_t)(sep - fid_buffer);
  if (len >= sizeof(p_buffer))
    return Error("\"%s\": %s", fid_buffer, strerror(ENAMETOOLONG));

  memcpy(p_buffer, fid_buffer, len);
  p_buffer[len] = '\0';

  bool exists;
  DAMGR_CALL(lstat_result(p_buffer, DIR_TYPE, &exists));

  if (exists) // parent already exists
    return Success;

  DAMGR_CALL(make_dir_parent_result(p_buffer));

  return damgr_mkdir_result(p_buffer, DIR_PERM);
}

Result_t damgr_make_dir_result(const char *fid_buffer, bool *exists) {
  DAMGR_CALL(lstat_result(fid_buffer, DIR_TYPE, exists));

  if (*exists) // folder already exists
    return Success;

  DAMGR_CALL(make_dir_parent_result(fid_buffer));

  return damgr_mkdir_result(fid_buffer, DIR_PERM);
}

Result_t damgr_dir_exists_result(const char *fid_buffer, bool *out) {
  DAMGR_CALL(lstat_result(fid_buffer, DIR_TYPE, out));
  return Success;
}

Result_t damgr_reg_exists_result(const char *fid_buffer, bool *out) {
  DAMGR_CALL(lstat_result(fid_buffer, REG_TYPE, out));
  return Success;
}

#endif /* DAMGR_UTILS_IMPLEMENTATION */
#endif /* DAMGR_UTILS_H */
