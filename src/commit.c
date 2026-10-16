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
  int ret;

  return EXIT_SUCCESS;
}

int damgr_commit(Damgr_Arena *arena) {
  int ret;
  char fid_buf[DAMGR_FID_BUF_MAX];
  snprintf(fid_buf, sizeof(fid_buf), "%s/.local/state/damgr/.backup",
           getenv("HOME"));
  // TODO: do we also want to clear the dir here?
  ret = damgr_make_dir(fid_buf);
  if (ret == EXIT_FAILURE)
    return ret;

  ret = backup_state();
  if (ret == EXIT_FAILURE)
    return ret;

  return EXIT_SUCCESS;
}
