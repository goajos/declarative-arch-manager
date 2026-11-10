#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include <dirent.h>
#include <string.h>

#include "damgr/paths.h"
#include "damgr/result.h"

#define TMP ".tmp"
#define JOURNAL ".journal"
constexpr mode_t DIR_PERM = 0755;
constexpr size_t COPY_BUFFER_MAX = 4096;

typedef enum filetype { DIR_TYPE, REG_TYPE } Damgr_Filetype;

Result_t damgr_make_dir_r(const char *fid_buffer, bool *exists);
Result_t damgr_is_dir_empty_r(const char *fid_buffer, bool *out);
Result_t damgr_dir_exists_r(const char *fid_buffer, bool *out);
Result_t damgr_reg_exists_r(const char *fid_buffer, bool *out);

Result_t damgr_copy_file_r(const char *src_buffer, const char *dst_buffer);
Result_t damgr_copy_dir_files_r(const char *src_buffer, const char *dst_buffer);
Result_t damgr_remove_dir_files_r(const char *dir_buffer);
Result_t damgr_remove_dir_r(const char *dir_buffer);

#ifdef DAMGR_UTILS_IMPLEMENTATION

static Result_t lstat_r(const char *fid_buffer, Damgr_Filetype type,
                        bool *out) {
  struct stat st;
  if (lstat(fid_buffer, &st) == 0) {
    bool matched =
        (type == DIR_TYPE) ? S_ISDIR(st.st_mode) : S_ISREG(st.st_mode);

    if (!matched)
      return Error(
          damgr_err_fmt("%s exists but is wrong file type", fid_buffer));
    // exists and is right file type, so true
    *out = true;
    return Success;
  }
  if (errno != ENOENT)
    return Error(
        damgr_err_fmt("lstat %s failed: %s", fid_buffer, strerror(errno)));
  // errno == ENOENT means the file doesn't exist, so false
  *out = false;
  return Success;
}

static Result_t make_dir_parent_r(const char *fid_buffer) {
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

  bool exists;
  DAMGR_CALL(lstat_r(p_buffer, DIR_TYPE, &exists));

  if (exists) // parent already exists
    return Success;

  DAMGR_CALL(make_dir_parent_r(p_buffer));

  return damgr_mkdir_r(p_buffer, DIR_PERM);
}

Result_t damgr_make_dir_r(const char *fid_buffer, bool *exists) {
  DAMGR_CALL(lstat_r(fid_buffer, DIR_TYPE, exists));

  if (*exists) // folder already exists
    return Success;

  DAMGR_CALL(make_dir_parent_r(fid_buffer));

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

Result_t damgr_dir_exists_r(const char *fid_buffer, bool *out) {
  DAMGR_CALL(lstat_r(fid_buffer, DIR_TYPE, out));
  return Success;
}

Result_t damgr_reg_exists_r(const char *fid_buffer, bool *out) {
  DAMGR_CALL(lstat_r(fid_buffer, REG_TYPE, out));
  return Success;
}

static Result_t is_state_file_r(const char *dir, const char *name,
                                char *fid_buffer, size_t size, bool *out) {
  *out = false;
  if (name[0] == '.') // skips ., .., .tmp, and .journal
    return Success;

  DAMGR_CALL(damgr_join_path_r(fid_buffer, size, dir, name));
  struct stat st;
  if (lstat(fid_buffer, &st) != 0)
    return Error(
        damgr_err_fmt("lstat %s failed: %s", fid_buffer, strerror(errno)));

  *out = S_ISREG(st.st_mode);
  return Success;
}

static Result_t copy_stream_r(FILE *src_fid, FILE *dst_fid,
                              const char *src_buffer, const char *dst_buffer) {
  unsigned char buffer[COPY_BUFFER_MAX];
  size_t n;
  while ((n = fread(buffer, 1, sizeof(buffer), src_fid)) > 0) {
    if (fwrite(buffer, 1, n, dst_fid) != n)
      return Error(
          damgr_err_fmt("could not write %s: %s", dst_buffer, strerror(errno)));
  }
  if (ferror(src_fid))
    return Error(damgr_err_fmt("could not read %s", src_buffer));
  if (fflush(dst_fid) != 0)
    return Error(
        damgr_err_fmt("could not flush %s: %s", dst_buffer, strerror(errno)));
  return Success;
}

Result_t damgr_copy_file_r(const char *src_buffer, const char *dst_buffer) {
  FILE *src_fid;
  DAMGR_CALL(damgr_fopen_r(src_buffer, "rb", &src_fid));
  FILE *dst_fid;
  // save result so we can ensure src fid is closed before return
  Result_t r = damgr_fopen_r(dst_buffer, "wb", &dst_fid);
  if (!r.is_success) {
    fclose(src_fid);
    return r;
  }

  // save result so we can ensure both fids are closed before return
  r = copy_stream_r(src_fid, dst_fid, src_buffer, dst_buffer);
  fclose(src_fid);
  fclose(dst_fid);
  return r;
}

static Result_t copy_dir_entries_r(DIR *dir, const char *src_buffer,
                                   const char *dst_buffer) {
  char src[PATH_BUFFER_MAX];
  char dst[PATH_BUFFER_MAX];
  bool is_file;
  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    DAMGR_CALL(
        is_state_file_r(src_buffer, ent->d_name, src, sizeof(src), &is_file));
    if (!is_file)
      continue;
    DAMGR_CALL(damgr_join_path_r(dst, sizeof(dst), dst_buffer, ent->d_name));
    DAMGR_CALL(damgr_copy_file_r(src, dst));
  }
  return Success;
}

Result_t damgr_copy_dir_files_r(const char *src_buffer,
                                const char *dst_buffer) {
  DIR *dir;
  DAMGR_CALL(damgr_opendir_r(src_buffer, &dir));
  // save result so we can ensure dir is closed before return
  Result_t r = copy_dir_entries_r(dir, src_buffer, dst_buffer);
  closedir(dir);
  return r;
}

static Result_t remove_dir_entries_r(DIR *dir, const char *dir_buffer) {
  char fid_buffer[PATH_BUFFER_MAX];
  bool is_file;
  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    DAMGR_CALL(is_state_file_r(dir_buffer, ent->d_name, fid_buffer,
                               sizeof(fid_buffer), &is_file));
    if (!is_file)
      continue;
    DAMGR_CALL(damgr_unlink_r(fid_buffer));
  }
  return Success;
}

Result_t damgr_remove_dir_files_r(const char *dir_buffer) {
  DIR *dir;
  DAMGR_CALL(damgr_opendir_r(dir_buffer, &dir));
  // save result so we can ensure dir is closed before return
  Result_t r = remove_dir_entries_r(dir, dir_buffer);
  closedir(dir);
  return r;
}

Result_t damgr_remove_dir_r(const char *dir_buffer) {
  DAMGR_CALL(damgr_remove_dir_files_r(dir_buffer));
  DAMGR_CALL(damgr_rmdir_r(dir_buffer));
  return Success;
}

#endif /* DAMGR_UTILS_IMPLEMENTATION */
#endif /* DAMGR_UTILS_H */
