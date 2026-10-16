#include "damgr/log.h"
#include "damgr/state.h"
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

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

static int backup_state_file(const char *srcbuf, const char *dstbuf) {
  if (access(srcbuf, F_OK) != 0) {
    damgr_log(INFO, "skipping backup of non-existent file: %s", srcbuf);
    return EXIT_SUCCESS;
  }
  return copy_file(srcbuf, dstbuf);
}

static int backup_state(const Damgr_Config *config) {
  int ret;
  char srcbuf[DAMGR_PATH_MAX];
  char dstbuf[DAMGR_PATH_MAX];
  snprintf(srcbuf, sizeof(srcbuf), "%s/.local/state/damgr/config_state.conf",
           getenv("HOME"));
  snprintf(dstbuf, sizeof(dstbuf),
           "%s/.local/state/damgr/.backup/config_state.conf", getenv("HOME"));
  ret = backup_state_file(srcbuf, dstbuf);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  snprintf(srcbuf, sizeof(srcbuf), "%s/.local/state/damgr/%s_state.conf",
           getenv("HOME"), config->active_host->host_name);
  snprintf(dstbuf, sizeof(dstbuf),
           "%s/.local/state/damgr/.backup/%s_state.conf", getenv("HOME"),
           config->active_host->host_name);
  ret = backup_state_file(srcbuf, dstbuf);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  for (size_t i = 0; i < config->state_host->modules_cnt; ++i) {
    snprintf(srcbuf, sizeof(srcbuf), "%s/.local/state/damgr/%s_state.conf",
             getenv("HOME"), config->active_host->modules[i]->module_name);
    snprintf(dstbuf, sizeof(dstbuf),
             "%s/.local/state/damgr/.backup/%s_state.conf", getenv("HOME"),
             config->active_host->modules[i]->module_name);
    ret = backup_state_file(srcbuf, dstbuf);
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
  snprintf(dstbuf, sizeof(dstbuf), "%s/.local/state/damgr/%s_state.conf",
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

static int rmdir_recursive(const char *fidbuf) {
  struct stat st;
  if (lstat(fidbuf, &st) != 0) {
    if (errno == ENOENT)
      return EXIT_SUCCESS;
    damgr_log(ERROR, "could not stat %s: %s", fidbuf, strerror(errno));
    return EXIT_FAILURE;
  }
  // unlink files
  if (!S_ISDIR(st.st_mode)) {
    if (unlink(fidbuf) != 0) {
      damgr_log(ERROR, "could not remove file %s: %s", fidbuf, strerror(errno));
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  DIR *open_dir = opendir(fidbuf);
  if (open_dir == nullptr) {
    damgr_log(ERROR, "could not open directory %s: %s", fidbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  // recursively search dirs
  struct dirent *ent;
  int ret = EXIT_SUCCESS;
  while ((ent = readdir(open_dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    char childbuf[DAMGR_PATH_MAX];
    snprintf(childbuf, sizeof(childbuf), "%s/%s", fidbuf, ent->d_name);
    if (rmdir_recursive(childbuf) != EXIT_SUCCESS) {
      closedir(open_dir);
      return EXIT_FAILURE;
    }
  }
  closedir(open_dir);

  if (rmdir(fidbuf) != 0) {
    damgr_log(ERROR, "could not remove directory %s: %s", fidbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }
  return ret;
}

int damgr_commit(const Damgr_Config *config) {
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

  // clear both tmp folders on success
  char stagingbuf[DAMGR_PATH_MAX];
  snprintf(stagingbuf, sizeof(stagingbuf), "%s/.local/state/damgr/.staging",
           getenv("HOME"));
  ret = rmdir_recursive(stagingbuf);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  ret = rmdir_recursive(backupbuf);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  damgr_log(INFO, "commit finished");
  return ret;
}
