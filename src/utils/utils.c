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

int damgr_init_dir(char *user, bool is_state) {
  struct stat st;
  char fidbuf[damgr_path_max];
  char *path =
      (is_state) ? "/home/%s/.local/state/damgr" : "/home/%s/.config/damgr";
  snprintf(fidbuf, sizeof(fidbuf), path, user);
  char *fmt = (is_state) ? "state" : "config";
  if (stat(fidbuf, &st) == -1) {
    if (errno == ENOENT) {
      if (mkdir(fidbuf, 0777) != -1) {
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

char *damgr_get_user() {
  struct passwd *pwd = getpwuid(geteuid());
  if (pwd == nullptr) {
    damgr_log(ERROR, "getpwuid failed: %s", strerror(errno));
    return nullptr;
  }
  return pwd->pw_name;
}

char *damgr_string_copy(char *str) {
  char *ret = nullptr;
  size_t len = strlen(str);
  if (len) {
    ret = malloc(len + 1);
    memcpy(ret, str, len);
    ret[len] = '\0'; // ensure proper null termination
  }
  return ret;
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

static int damgr_execute_execv(char *path, char **argv) {
  pid_t pid = fork();
  if (pid == -1) {
    damgr_log(ERROR, "fork failed: %s", strerror(errno));
    return EXIT_FAILURE;
  } else if (pid == 0) {
    execv(path, argv);
  } else {
    int wstatus;
    if (waitpid(pid, &wstatus, 0) != pid) {
      damgr_log(ERROR, "execv waitpid failed: %s", strerror(errno));
      return EXIT_FAILURE;
    }
    if (WIFEXITED(wstatus)) {
      if (WEXITSTATUS(wstatus) != EXIT_SUCCESS) {
        damgr_log(ERROR, "execv failed with exit code: %d",
                  WEXITSTATUS(wstatus));
        return EXIT_FAILURE;
      }
    }
    if (WIFSIGNALED(wstatus)) {
      damgr_log(ERROR, "execv was terminated by signal: %d", WTERMSIG(wstatus));
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int damgr_execute_aur_update_command(char *aur_helper) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf), "/usr/bin/%s", aur_helper);
  char **argv = malloc(3 * sizeof(char *));
  argv[0] = fidbuf;
  argv[1] = "-Syu";
  argv[2] = nullptr;
  int ret = damgr_execute_execv(fidbuf, argv);
  free(argv);
  return ret;
}

int damgr_execute_update_command() {
  char **argv = malloc(4 * sizeof(char *));
  argv[0] = "sudo";
  argv[1] = "pacman";
  argv[2] = "-Syu";
  argv[3] = nullptr;
  int ret = damgr_execute_execv("/usr/bin/sudo", argv);
  free(argv);
  return ret;
}
