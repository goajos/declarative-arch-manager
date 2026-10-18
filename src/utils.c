#define _DEFAULT_SOURCE
#include "damgr/utils.h"

static const char *active_dir_type_string(const Damgr_Active_Dir_Type type) {
  switch (type) {
  case CONTEXT:
    return ".config/damgr";
  case HOST:
    return ".config/damgr/hosts";
  case MODULE:
    return ".config/damgr/modules";
  case DOTFILE:
    return ".config/damgr/dotfiles";
  case NO_ACTIVE_DIR:
    return "";
  }
  return "";
}

const char *state_dir_type_string(const Damgr_State_Dir_Type type) {
  switch (type) {
  case STATE:
    return ".local/state/damgr";
  case STAGING:
    return ".local/state/damgr/.staging";
  case BACKUP:
    return ".local/state/damgr/.backup";
  case NO_STATE_DIR:
    return "";
  }
  return "";
}

void damgr_format_fid_buf(char *fid_buf, const char *name, const bool is_state,
                          const Damgr_Active_Dir_Type active_dir_type,
                          const Damgr_State_Dir_Type state_dir_type) {
  if (is_state) {
    if (state_dir_type == STAGING) {
      snprintf(fid_buf, DAMGR_FID_BUF_MAX, "%s/%s/%s.conf", getenv("HOME"),
               state_dir_type_string(state_dir_type), name);
    } else {
      snprintf(fid_buf, DAMGR_FID_BUF_MAX, "%s/%s/state_%s.conf",
               getenv("HOME"), state_dir_type_string(state_dir_type), name);
    }
  } else {
    snprintf(fid_buf, DAMGR_FID_BUF_MAX, "%s/%s/%s.conf", getenv("HOME"),
             active_dir_type_string(active_dir_type), name);
  }
}

void damgr_trim_string_inplace(char *str) {
  if (*str == '\0')
    return;

  char *start = str;
  while (isspace((unsigned char)*start)) {
    ++start;
  }

  char *end = str + strlen(str) - 1;
  while (end > start && isspace((unsigned char)*end)) {
    --end;
  }

  *(end + 1) = '\0'; // ensure proper null termination

  if (start != str) {
    memmove(str, start, end - start + 2); // +2 includes the null terminator
  }
}

static bool make_dir_parents(const char *fid_buf) {
  const char *sep = strrchr(fid_buf, '/');
  if (sep == nullptr || sep == fid_buf)
    return true;

  char parent[DAMGR_FID_BUF_MAX];
  size_t len = (size_t)(sep - fid_buf);
  if (len >= sizeof(parent)) {
    return false;
  }
  memcpy(parent, fid_buf, len);
  parent[len] = '\0';

  struct stat st;
  if (lstat(parent, &st) == 0) {
    if (S_ISDIR(st.st_mode))
      return true;
    damgr_log(ERROR, "%s exists but is not a dir", parent);
    return false;
  }
  if (errno != ENOENT) {
    damgr_log(ERROR, "could not stat %s: %s", parent, strerror(errno));
    return false;
  }

  if (!make_dir_parents(parent))
    return false;

  if (mkdir(parent, DAMGR_DIR_PERM) != 0) {
    damgr_log(ERROR, "could not create %s dir: %s", parent, strerror(errno));
    return false;
  }
  return true;
}

bool damgr_make_dir(const char *fid_buf) {
  struct stat st;
  if (lstat(fid_buf, &st) == 0) {
    if (S_ISDIR(st.st_mode))
      return true;
    damgr_log(ERROR, "%s exists but is not a dir", fid_buf);
    return false;
  }
  if (errno != ENOENT) {
    damgr_log(ERROR, "could not stat %s: %s", fid_buf, strerror(errno));
    return false;
  }

  if (!make_dir_parents(fid_buf))
    return false;

  if (mkdir(fid_buf, DAMGR_DIR_PERM) != 0) {
    damgr_log(ERROR, "could not create %s dir: %s", fid_buf, strerror(errno));
    return false;
  }
  damgr_log(INFO, "created %s dir", fid_buf);
  return true;
}

bool damgr_dir_exists(const char *fid_buf) {
  struct stat st;

  if (lstat(fid_buf, &st) != 0) {
    if (errno == ENOENT) {
      // damgr_log(ERROR, "dir %s does not exist: %s", fid_buf,
      // strerror(errno));
      return false;
    }
    damgr_log(ERROR, "could not stat %s: %s", fid_buf, strerror(errno));
    return false;
  }

  return S_ISDIR(st.st_mode);
}

bool damgr_is_dir_empty(const char *fid_buf) {
  DIR *dir = opendir(fid_buf);
  if (dir == nullptr) {
    damgr_log(ERROR, "could not open dir %s: %s", fid_buf, strerror(errno));
    return false;
  }

  int count = 0;
  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;

    if (strcmp(ent->d_name, DAMGR_JOURNAL) == 0)
      continue; // the journal is not state content

    if (ent->d_type == DT_REG)
      count++;
  }

  closedir(dir);
  return count == 0;
}

int damgr_copy_file(const char *src_buf, const char *dst_buf) {
  FILE *src_fid = fopen(src_buf, "rb");
  if (src_fid == nullptr) {
    damgr_log(ERROR, "could not open src fid %s for copying: %s", src_buf,
              strerror(errno));
    return EXIT_FAILURE;
  }
  FILE *dst_fid = fopen(dst_buf, "wb");
  if (dst_fid == nullptr) {
    damgr_log(ERROR, "could not open dst fid %s for copying: %s", dst_buf,
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

int damgr_copy_dir(const char *src_buf, const char *dst_buf,
                   const bool rename) {
  return damgr_copy_dir_except(src_buf, dst_buf, rename, nullptr);
}

int damgr_copy_dir_except(const char *src_buf, const char *dst_buf,
                          const bool rename, const char *keep) {
  DIR *src_dir = opendir(src_buf);
  if (src_dir == nullptr) {
    damgr_log(ERROR, "could not open src dir %s: %s", src_buf, strerror(errno));
    return EXIT_FAILURE;
  }

  struct dirent *ent;
  int ret = EXIT_SUCCESS;
  while ((ent = readdir(src_dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (keep != nullptr && strcmp(ent->d_name, keep) == 0)
      continue;

    char src_file_buf[DAMGR_FID_BUF_MAX + sizeof(ent->d_name)];
    size_t dst_file_buf_size =
        rename ? DAMGR_FID_BUF_MAX + sizeof(ent->d_name) + sizeof("state_")
               : DAMGR_FID_BUF_MAX + sizeof(ent->d_name);
    char dst_file_buf[dst_file_buf_size];
    snprintf(src_file_buf, sizeof(src_file_buf), "%s/%s", src_buf, ent->d_name);
    if (rename) {
      snprintf(dst_file_buf, sizeof(dst_file_buf), "%s/%s%s", dst_buf, "state_",
               ent->d_name);
    } else {
      snprintf(dst_file_buf, sizeof(dst_file_buf), "%s/%s", dst_buf,
               ent->d_name);
    }

    struct stat st;
    if (lstat(src_file_buf, &st) != 0 || !S_ISREG(st.st_mode))
      continue;

    ret = damgr_copy_file(src_file_buf, dst_file_buf);
    if (ret == EXIT_FAILURE)
      break;
  }

  closedir(src_dir);
  return ret;
}

int damgr_clean_dir(const char *fid_buf, const bool state) {
  return damgr_clean_dir_except(fid_buf, state, nullptr);
}

int damgr_clean_dir_except(const char *fid_buf, const bool state,
                           const char *keep) {
  DIR *open_dir = opendir(fid_buf);
  if (open_dir == nullptr) {
    damgr_log(ERROR, "could not open directory %s: %s", fid_buf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  struct stat st;
  struct dirent *ent;
  while ((ent = readdir(open_dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (keep != nullptr && strcmp(ent->d_name, keep) == 0)
      continue; // never remove the reserved file
    char child_buf[DAMGR_FID_BUF_MAX + sizeof(ent->d_name)];
    snprintf(child_buf, sizeof(child_buf), "%s/%s", fid_buf, ent->d_name);
    if (lstat(child_buf, &st) != 0) {
      damgr_log(ERROR, "could not stat %s: %s", child_buf, strerror(errno));
      return EXIT_FAILURE;
    }
    // unlink files
    if (S_ISREG(st.st_mode)) {
      if (unlink(child_buf) != 0) {
        damgr_log(ERROR, "could not remove file %s: %s", child_buf,
                  strerror(errno));
        return EXIT_FAILURE;
      }
    }
  }
  closedir(open_dir);

  if (!state) {
    if (rmdir(fid_buf) != 0) {
      damgr_log(ERROR, "could not remove directory %s: %s", fid_buf,
                strerror(errno));
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
