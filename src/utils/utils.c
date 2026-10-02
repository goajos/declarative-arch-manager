#define _POSIX_C_SOURCE 200112L

#include "damgr/utils.h"
#include "damgr/log.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

const int damgr_path_max = 4096;

int damgr_is_state_dir_empty(char *dir) {
  DIR *open_dir = opendir(dir);
  if (open_dir == nullptr) {
    damgr_log(ERROR, "could not open directory %s: %s", dir, strerror(errno));
    return EXIT_FAILURE;
  }

  int count = 0;
  struct dirent *ent;
  errno = 0;
  while ((ent = readdir(open_dir)) != nullptr) {
    if (errno != 0) {
      damgr_log(ERROR, "could not read directory %s: %s", open_dir,
                strerror(errno));
      closedir(open_dir);
      return EXIT_FAILURE;
    }
    ++count;
  }
  closedir(open_dir);
  return count;
}

static int mkdir_p(char *path) {
  char *_path = nullptr;
  char *p;
  mode_t mode = 0777;

  _path = strdup(path);
  if (_path == nullptr) {
    return EXIT_FAILURE;
  }

  int ret = EXIT_SUCCESS;
  for (p = _path + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(_path, mode) != 0 && errno != EEXIST) {
        ret = EXIT_FAILURE;
        break;
      }
      *p = '/';
    }
  }

  if (ret == EXIT_SUCCESS && mkdir(_path, mode) != 0 && errno != EEXIST) {
    ret = EXIT_FAILURE;
  }

  free(_path);
  return ret;
}

int damgr_init_dir(char *user, bool is_state) {
  struct stat st;
  char fidbuf[damgr_path_max];
  char *path =
      (is_state) ? "/home/%s/.local/state/damgr" : "/home/%s/.config/damgr";
  snprintf(fidbuf, sizeof(fidbuf), path, user);
  char *fmt = (is_state) ? "state" : "config";
  if (stat(fidbuf, &st) == -1) {
    if (errno == ENOENT) {
      if (mkdir_p(fidbuf) != -1) {
        damgr_log(INFO, "successfully created damgr %s directory: %s", fmt,
                  fidbuf);
      } else {
        damgr_log(ERROR, "mkdir %s failed: %s", fidbuf, strerror(errno));
        return EXIT_FAILURE;
      }
    } else {
      damgr_log(ERROR, "stat %s failed: %s", fidbuf, strerror(errno));
      return EXIT_FAILURE;
    }
  } else {
    damgr_log(ERROR, "damgr %s directory already exists", fidbuf);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

bool damgr_string_contains(char *haystack, char *needle) {
  bool contains = false;
  for (size_t i = 0, j = 0; i < strlen(haystack) && !contains; ++i) {
    while (haystack[i] == needle[j]) {
      ++j;
      ++i;
      if (j == strlen(needle)) {
        contains = true;
        return contains;
      }
    }
    j = 0;
  }
  return contains;
}

void damgr_string_trim(char *str) {
  if (*str == '\0' || *str == '\n') {
    return;
  }
  char *start = str;
  while (isspace((char)*start)) {
    ++start;
  }
  char *end = str + strlen(str) - 1;
  while (end > start && isspace((char)*end)) {
    --end;
  }
  *(end + 1) = '\0'; // ensure proper null termination

  if (start != str) {
    memmove(str, start, end - start + 2); // +2 includes the null terminator
  }
}

int damgr_get_conf_key(char *key) {
  for (int i = 0; i < MAX_CONF_KEY; ++i) {
    if (strcmp(damgr_conf_keys[i], key) == 0) {
      return i;
    }
  }
  return -1;
}

int damgr_qcharcmp(const void *p1, const void *p2) {
  return strcmp(*(const char **)p1, *(const char **)p2);
}
