#include "damgr/utils.h"
#include "damgr/state.h"
#include <dirent.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int is_damgr_state_dir_empty(char *dir) {
  DIR *open_dir = opendir(dir);
  if (open_dir == nullptr) {
    return EXIT_FAILURE;
  }

  int cnt = 0;
  struct dirent *ent;
  while ((ent = readdir(open_dir)) != nullptr) {
    ++cnt;
  }
  closedir(open_dir);
  return cnt;
}

char *get_user() {
  struct passwd *pwd = getpwuid(geteuid());
  return pwd->pw_name;
}

size_t read_func(void *user_data, char *buf, size_t bufsize) {
  FILE *fid = (FILE *)user_data;
  return fread(buf, 1, bufsize, fid);
}

size_t write_func(void *user_data, char const *data, size_t nbytes) {
  FILE *fid = (FILE *)user_data;
  return fwrite(data, 1, nbytes, fid);
}

char *string_copy(char *str) {
  char *ret = nullptr;
  size_t len = strlen(str);
  if (len) {
    ret = malloc(len + 1);
    memcpy(ret, str, len);
    ret[len] = '\0'; // ensure proper null termination
  }
  return ret;
}

bool string_contains(char *haystack, char *needle) {
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

int qcharcmp(const void *p1, const void *p2) {
  return strcmp(*(const char **)p1, *(const char **)p2);
}

int execute_package_install_command(struct darray packages) {
  pid_t pid = fork();
  if (pid == -1)
    return EXIT_FAILURE;
  if (pid == 0) {
    char **argv = malloc((packages.count + 5) * sizeof(char *));
    argv[0] = "sudo";
    argv[1] = "pacman";
    argv[2] = "-S";
    argv[3] = "--needed";
    for (size_t i = 0; i < packages.count; ++i) {
      argv[4 + i] = packages.items[i];
    }
    argv[4 + packages.count] = nullptr;
    execv("/usr/bin/sudo", argv);
  } else {
    waitpid(pid, nullptr, 0);
  }
  return EXIT_SUCCESS;
}

int execute_aur_package_install_command(struct darray packages,
                                        char *aur_helper) {
  char fidbuf[PATH_MAX];
  snprintf(fidbuf, sizeof(fidbuf), "/usr/bin/%s", aur_helper);
  pid_t pid = fork();
  if (pid == -1)
    return EXIT_FAILURE;
  if (pid == 0) {
    char **argv = malloc((packages.count + 4) * sizeof(char *));
    argv[0] = aur_helper;
    argv[1] = "-S";
    argv[2] = "--needed";
    for (size_t i = 0; i < packages.count; ++i) {
      argv[3 + i] = packages.items[i];
    }
    argv[3 + packages.count] = nullptr;
    execv(fidbuf, argv);
  } else {
    waitpid(pid, nullptr, 0);
  }
  return EXIT_SUCCESS;
}

int execute_package_remove_command(struct darray packages) {
  pid_t pid = fork();
  if (pid == -1)
    return EXIT_FAILURE;
  if (pid == 0) {
    char **argv = malloc((packages.count + 5) * sizeof(char *));
    argv[0] = "sudo";
    argv[1] = "pacman";
    argv[2] = "-Rns";
    for (size_t i = 0; i < packages.count; ++i) {
      argv[3 + i] = packages.items[i];
    }
    argv[3 + packages.count] = nullptr;
    execv("/usr/bin/sudo", argv);
  } else {
    waitpid(pid, nullptr, 0);
  }
  return EXIT_SUCCESS;
}

int execute_hook_command(bool privileged, char *hook) {
  pid_t pid = fork();
  if (pid == -1)
    return EXIT_FAILURE;
  if (pid == 0) {
    char fidbuf[PATH_MAX];
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damgr/hooks/%s",
             get_user(), hook);
    if (privileged) {
      execl("/usr/bin/sudo", "sudo", fidbuf, nullptr);
    } else {
      execl(fidbuf, hook, nullptr);
    }
  } else {
    waitpid(pid, nullptr, 0);
  }
  return EXIT_SUCCESS;
}

int execute_service_command(bool privileged, bool to_enable, char *service) {
  pid_t pid = fork();
  if (pid == -1)
    return EXIT_FAILURE;
  if (pid == 0) {
    char **argv = malloc(5 * sizeof(char *));
    if (privileged) {
      argv[0] = "sudo";
      argv[1] = "systemctl";
      argv[2] = (to_enable) ? "enable" : "disable";
      argv[3] = service;
      argv[4] = nullptr;
      execv("/usr/bin/sudo", argv);
    } else {
      argv[0] = "systemctl";
      argv[1] = "--user";
      argv[2] = (to_enable) ? "enable" : "disable";
      argv[3] = service;
      argv[4] = nullptr;
      execv("/usr/bin/systemctl", argv);
    }
  } else {
    waitpid(pid, nullptr, 0);
  }
  return EXIT_SUCCESS;
}

int execute_dotfile_command(bool to_link, char *dotfile) {
  pid_t pid = fork();
  if (pid == -1)
    return EXIT_FAILURE;
  if (pid == 0) {
    char src_fidbuf[PATH_MAX];
    snprintf(src_fidbuf, sizeof(src_fidbuf),
             "/home/%s/.config/damgr/dotfiles/%s", get_user(), dotfile);
    char dst_fidbuf[PATH_MAX];
    snprintf(dst_fidbuf, sizeof(dst_fidbuf), "/home/%s/.config/%s", get_user(),
             dotfile);
    if (to_link) {
      execl("/usr/bin/ln", "ln", "--symbolic", src_fidbuf, dst_fidbuf, nullptr);
    } else {
      execl("/usr/bin/rm", "rm", dst_fidbuf, nullptr);
    }
  } else {
    waitpid(pid, nullptr, 0);
  }
  return EXIT_SUCCESS;
}

int execute_aur_update_command(char *aur_helper) {
  char fidbuf[PATH_MAX];
  snprintf(fidbuf, sizeof(fidbuf), "/usr/bin/%s", aur_helper);
  pid_t pid = fork();
  if (pid == -1)
    return EXIT_FAILURE;
  if (pid == 0) {
    char **argv = malloc(3 * sizeof(char *));
    argv[0] = fidbuf;
    argv[1] = "-Syu";
    argv[2] = nullptr;
    execv(fidbuf, argv);
  } else {
    waitpid(pid, nullptr, 0);
  }
  return EXIT_SUCCESS;
}

int execute_update_command() {
  pid_t pid = fork();
  if (pid == -1)
    return EXIT_FAILURE;
  if (pid == 0) {
    char **argv = malloc(4 * sizeof(char *));
    argv[0] = "sudo";
    argv[1] = "pacman";
    argv[2] = "-Syu";
    argv[3] = nullptr;
    execv("/usr/bin/sudo", argv);
  } else {
    waitpid(pid, nullptr, 0);
  }
  return EXIT_SUCCESS;
}
