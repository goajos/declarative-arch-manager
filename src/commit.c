#define _DEFAULT_SOURCE
#include "damgr/commit.h"
#include "damgr/utils.h"

static int copy_file(const char *src_buf, const char *dst_buf) {
  FILE *src_fid = fopen(src_buf, "rb");
  if (src_fid == nullptr) {
    damgr_log(ERROR, "could not open srcfid %s for copying: %s", src_buf,
              strerror(errno));
    return EXIT_FAILURE;
  }
  FILE *dst_fid = fopen(dst_buf, "wb");
  if (dst_fid == nullptr) {
    damgr_log(ERROR, "could not open dstfid %s for copying: %s", dst_buf,
              strerror(errno));
    fclose(src_fid);
    return EXIT_FAILURE;
  }

  unsigned char bytes_buf[DAMGR_BYTES_BUF_MAX];
  size_t bytes_read;
  while ((bytes_read = fread(bytes_buf, 1, sizeof(bytes_buf), src_fid)) > 0) {
    size_t bytes_written = fwrite(bytes_buf, 1, bytes_read, dst_fid);
    if (bytes_written != bytes_read) {
      damgr_log(ERROR, "could not copy bytes from  %s into %s", src_buf,
                dst_buf);
      fclose(src_fid);
      fclose(dst_fid);
      return EXIT_FAILURE;
    }
  }

  fclose(src_fid);
  fclose(dst_fid);
  return EXIT_SUCCESS;
}

static int backup_state() {
  char state_buf[DAMGR_FID_BUF_MAX];
  char backup_buf[DAMGR_FID_BUF_MAX];
  snprintf(state_buf, sizeof(state_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  snprintf(backup_buf, sizeof(backup_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(BACKUP));

  DIR *state_dir = opendir(state_buf);
  if (state_dir == nullptr) {
    damgr_log(ERROR, "could not open state dir %s: %s", state_buf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  struct dirent *ent;
  int ret = EXIT_SUCCESS;
  while ((ent = readdir(state_dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;

    char src_path[DAMGR_FID_BUF_MAX + sizeof(ent->d_name)];
    char dst_path[DAMGR_FID_BUF_MAX + sizeof(ent->d_name)];
    snprintf(src_path, sizeof(src_path), "%s/%s", state_buf, ent->d_name);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", backup_buf, ent->d_name);

    struct stat st;
    if (lstat(src_path, &st) != 0 || !S_ISREG(st.st_mode))
      continue;

    ret = copy_file(src_path, dst_path);
    if (ret == EXIT_FAILURE)
      break;
  }

  closedir(state_dir);
  return ret;
}

int damgr_commit() {
  char fid_buf[DAMGR_FID_BUF_MAX];
  snprintf(fid_buf, sizeof(fid_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STATE));
  // state folder should exist before commit
  if (!damgr_dir_exists(fid_buf)) {
    damgr_log(ERROR, "trying to commit without a state folder");
    return EXIT_FAILURE;
  }
  // state folder shouldn't be empty before commit
  if (damgr_is_dir_empty(fid_buf)) {
    damgr_log(ERROR, "trying to commit with an empty state folder");
    return EXIT_FAILURE;
  }

  // staging folder should exist before commit
  snprintf(fid_buf, sizeof(fid_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(STAGING));
  if (!damgr_dir_exists(fid_buf)) {
    damgr_log(ERROR, "trying to commit without a .staging folder");
    return EXIT_FAILURE;
  }
  // staging folder shouldn't be empty before commit
  if (damgr_is_dir_empty(fid_buf)) {
    damgr_log(ERROR, "trying to commit with an empty .staging folder");
    return EXIT_FAILURE;
  }

  int ret;
  snprintf(fid_buf, sizeof(fid_buf), "%s/%s", getenv("HOME"),
           state_dir_type_string(BACKUP));
  // TODO: do we also want to clear the dir here?
  ret = damgr_make_dir(fid_buf);
  if (ret == EXIT_FAILURE)
    return ret;

  ret = backup_state();
  if (ret == EXIT_FAILURE)
    return ret;

  return EXIT_SUCCESS;
}
