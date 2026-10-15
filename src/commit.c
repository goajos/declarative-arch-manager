#include "damgr/log.h"
#include "damgr/state.h"
#include <errno.h>
#include <sys/stat.h>

static int copy_file(const char *srcbuf, const char *dstbuf) {
  FILE *src_fid = fopen(srcbuf, "rb");
  if (src_fid == nullptr) {
    damgr_log(ERROR, "could not open srcfid %s for copying: %s", srcbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }
  FILE *dst_fid = fopen(dstbuf, "wb");
  if (dst_fid == nullptr) {
    damgr_log(ERROR, "could not open dstfid %s for copying: %s", dstbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  unsigned char buffer[DAMGR_BUFFER_MAX];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_fid)) > 0) {
    size_t bytes_written = fwrite(buffer, 1, bytes_read, dst_fid);
    if (bytes_written != bytes_read) {
      damgr_log(ERROR, "could not copy bytes from  %s into %s", srcbuf, dstbuf);
      fclose(src_fid);
      fclose(dst_fid);
      return EXIT_FAILURE;
    }
  }

  fclose(src_fid);
  fclose(dst_fid);

  return EXIT_SUCCESS;
}

static int backup_state(const Damgr_Config *config) {
  int ret;
  char srcbuf[DAMGR_PATH_MAX];
  char dstbuf[DAMGR_PATH_MAX];
  snprintf(srcbuf, sizeof(srcbuf), "%s/.local/state/damgr/config_state.conf",
           getenv("HOME"));
  snprintf(dstbuf, sizeof(dstbuf),
           "%s/.local/state/damgr/.backup/config_state.conf", getenv("HOME"));
  ret = copy_file(srcbuf, dstbuf);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  snprintf(srcbuf, sizeof(srcbuf), "%s/.local/state/damgr/%s_state.conf",
           getenv("HOME"), config->active_host->host_name);
  snprintf(srcbuf, sizeof(srcbuf),
           "%s/.local/state/damgr/.backup/%s_state.conf", getenv("HOME"),
           config->active_host->host_name);
  ret = copy_file(srcbuf, dstbuf);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  for (size_t i = 0; i < config->active_host->modules_cnt; ++i) {
    snprintf(srcbuf, sizeof(srcbuf), "%s/.local/state/damgr/%s_state.conf",
             getenv("HOME"), config->active_host->modules[i]->module_name);
    snprintf(dstbuf, sizeof(dstbuf),
             "%s/.local/state/damgr/.backup/%s_state.conf", getenv("HOME"),
             config->active_host->modules[i]->module_name);
    ret = copy_file(srcbuf, dstbuf);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
  }

  return ret;
}

static int rename_staging(const Damgr_Config *config) {
  int ret;
  char srcbuf[DAMGR_PATH_MAX];
  char dstbuf[DAMGR_PATH_MAX];
  snprintf(srcbuf, sizeof(srcbuf),
           "%s/.local/state/damgr/.staging/config_state.conf", getenv("HOME"));
  snprintf(dstbuf, sizeof(dstbuf), "%s/.local/state/damgr/config_state.conf",
           getenv("HOME"));
  ret = rename(srcbuf, dstbuf);
  if (ret == EXIT_FAILURE) {
    damgr_log(ERROR, "could not move staging file %s into place: %s", srcbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  snprintf(srcbuf, sizeof(srcbuf),
           "%s/.local/state/damgr/.staging/%s_state.conf", getenv("HOME"),
           config->active_host->host_name);
  snprintf(srcbuf, sizeof(srcbuf), "%s/.local/state/damgr/%s_state.conf",
           getenv("HOME"), config->active_host->host_name);
  ret = rename(srcbuf, dstbuf);
  if (ret == EXIT_FAILURE) {
    damgr_log(ERROR, "could not move host staging file %s into place: %s",
              srcbuf, strerror(errno));
    return EXIT_FAILURE;
  }

  for (size_t i = 0; i < config->active_host->modules_cnt; ++i) {
    snprintf(srcbuf, sizeof(srcbuf),
             "%s/.local/state/damgr/.staging/%s_state.conf", getenv("HOME"),
             config->active_host->modules[i]->module_name);
    snprintf(dstbuf, sizeof(dstbuf), "%s/.local/state/damgr/%s_state.conf",
             getenv("HOME"), config->active_host->modules[i]->module_name);
    ret = rename(srcbuf, dstbuf);
    if (ret == EXIT_FAILURE) {
      damgr_log(ERROR, "could not move module staging file %s into place: %s",
                srcbuf, strerror(errno));
      return EXIT_FAILURE;
    }
  }

  return ret;
}

// commit fills a .backup folder with the current state present
// then tries to rename .staging into current state
// on success clear both .staging and .backup
int damgr_commit(const Damgr_Config *config) {
  // TODO: clean the .backup folder before writing again?
  char backupbuf[DAMGR_PATH_MAX];
  snprintf(backupbuf, sizeof(backupbuf), "%s/.local/state/damgr/.backup",
           getenv("HOME"));
  if (mkdir(backupbuf, 0755) == -1) {
    damgr_log(ERROR, "could not create .backup dir: %s", strerror(errno));
    return EXIT_FAILURE;
  }

  int ret;
  ret = backup_state(config);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  ret = rename_staging(config);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  // TODO: on success clear the .backup and .staging folders recursively

  damgr_log(INFO, "commit finished");
  return ret;
}
