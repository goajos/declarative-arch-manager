#include "damgr/utils.h"

static const char *active_dir_type_string(Damgr_Active_Dir_Type type) {
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

const char *state_dir_type_string(Damgr_State_Dir_Type type) {
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
    if (state_dir_type == STAGING || state_dir_type == BACKUP) {
      snprintf(fid_buf, DAMGR_FID_BUF_MAX, "%s/%s/%s.conf", getenv("HOME"),
               state_dir_type_string(state_dir_type), name);
    } else {
      snprintf(fid_buf, DAMGR_FID_BUF_MAX, "%s/%s/%s_state.conf",
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

bool damgr_make_dir(const char *fid_buf) {
  struct stat st;
  if (lstat(fid_buf, &st) != 0) {
    if (errno == ENOENT) {
      if (mkdir(fid_buf, DAMGR_DIR_PERM) != 0) {
        damgr_log(ERROR, "could not create %s dir: %s", fid_buf,
                  strerror(errno));
        return false;
      }
      damgr_log(INFO, "created %s dir", fid_buf);
      return true;
    }
    damgr_log(ERROR, "could not stat %s: %s", fid_buf, strerror(errno));
    return false;
  }

  if (S_ISDIR(st.st_mode)) {
    damgr_log(INFO, "%s dir already exists", fid_buf);
    return true;
  }

  damgr_log(ERROR, "%s already exists but is not a dir", fid_buf);
  return false;
}

bool damgr_dir_exists(const char *fid_buf) {
  struct stat st;

  if (lstat(fid_buf, &st) != 0) {
    if (errno == ENOENT) {
      damgr_log(ERROR, "dir %s does not exist: %s", fid_buf, strerror(errno));
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

    count++;
  }

  closedir(dir);
  return count == 0;
}
