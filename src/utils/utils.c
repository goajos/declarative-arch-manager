#define _POSIX_C_SOURCE 200112L

#include "damgr/utils.h"
#include "damgr/actions.h"
#include "damgr/log.h"
#include "damgr/state.h"
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int is_damgr_state_dir_empty(char *dir) {
  DIR *open_dir = opendir(dir);
  if (open_dir == nullptr) {
    return EXIT_FAILURE;
  }

  int count = 0;
  struct dirent *ent;
  while ((ent = readdir(open_dir)) != nullptr) {
    ++count;
  }
  closedir(open_dir);
  return count;
}

int init_damgr_dir(char *user, bool is_state) {
  struct stat st;
  char fidbuf[PATH_MAX];
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
        damgr_log(ERROR, "mkdir %s failed: %s", fidbuf, errno);
        return EXIT_FAILURE;
      }
    } else {
      damgr_log(ERROR, "stat %s failed: %s", fidbuf, errno);
      return EXIT_FAILURE;
    }
  } else {
    damgr_log(ERROR, "damgr %s directory already exists", fidbuf);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

char *get_user() {
  struct passwd *pwd = getpwuid(geteuid());
  if (pwd == nullptr) {
    damgr_log(ERROR, "getpwuid failed: %s", errno);
    return nullptr;
  }
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
    free(argv);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
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
    free(argv);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
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
    free(argv);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
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
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
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
    free(argv);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int execute_dotfile_command(bool to_link, char *dotfile) {
  char src_fidbuf[PATH_MAX];
  snprintf(src_fidbuf, sizeof(src_fidbuf), "/home/%s/.config/damgr/dotfiles/%s",
           get_user(), dotfile);
  char dst_fidbuf[PATH_MAX];
  snprintf(dst_fidbuf, sizeof(dst_fidbuf), "/home/%s/.config/%s", get_user(),
           dotfile);

  if (to_link) {
    struct stat st;
    if (lstat(dst_fidbuf, &st) == 0) { // dst file exists
      if (S_ISLNK(st.st_mode)) {
        // dotfile symbolic link already exists
        return EXIT_SUCCESS;
      }
    }
  } else {
    unlink(dst_fidbuf);
    return EXIT_SUCCESS;
  }

  // dotfile symbolic link doesn't exist
  pid_t pid = fork();
  if (pid == -1)
    return EXIT_FAILURE;
  if (pid == 0) {
    execl("/usr/bin/ln", "ln", "--symbolic", src_fidbuf, dst_fidbuf, nullptr);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
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
    free(argv);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
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
    free(argv);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

void report_module_actions(struct module module, bool is_state) {
  char *fmt = (is_state) ? "state" : "config";
  damgr_log(ERROR, "failed to do module actions for %s module: %s", fmt,
            module.name);
  for (size_t i = 0; i < module.module_actions.count; ++i) {
    struct action action = module.module_actions.items[i];
    damgr_log(ERROR, "  [%s] %s: %s", action_status_names[action.status],
              action_type_names[action.type], action.payload.name);
  }
}

static void free_darray(struct darray array) {
  for (size_t i = 0; i < array.count; ++i) {
    char *item = array.items[i];
    free(item);
    item = nullptr;
  }
  free_sized(array.items, array.capacity * sizeof(*array.items));
  array.items = nullptr;
}

static void free_module(struct module module) {
  free(module.name);
  module.name = nullptr;
  if (module.pre_root_hooks.capacity > 0) {
    free_darray(module.pre_root_hooks);
  }
  if (module.pre_user_hooks.capacity > 0) {
    free_darray(module.pre_user_hooks);
  }
  if (module.packages.capacity > 0) {
    free_darray(module.packages);
  }
  if (module.aur_packages.capacity > 0) {
    free_darray(module.aur_packages);
  }
  if (module.user_services.capacity > 0) {
    free_darray(module.user_services);
  }
  if (module.post_root_hooks.capacity > 0) {
    free_darray(module.post_root_hooks);
  }
  if (module.post_user_hooks.capacity > 0) {
    free_darray(module.post_user_hooks);
  }
  if (module.module_actions.capacity > 0) {
    for (size_t i = 0; i < module.module_actions.count; ++i) {
      module.module_actions.items[i].payload.name = nullptr;
      for (size_t j = 0;
           j < module.module_actions.items[i].payload.packages.count; ++j) {
        module.module_actions.items[i].payload.packages.items[j] = nullptr;
      }
    }
  }
}

static void free_host(struct host host) {
  free(host.name);
  host.name = nullptr;
  if (host.root_services.capacity > 0) {
    free_darray(host.root_services);
  }
  for (size_t i = 0; i < host.modules.count; ++i) {
    if (host.modules.items[i].name != nullptr) {
      free_module(host.modules.items[i]);
    }
  }
  if (host.host_actions.capacity > 0) {
    for (size_t i = 0; i < host.host_actions.count; ++i) {
      host.host_actions.items[i].payload.name = nullptr;
      for (size_t j = 0; j < host.host_actions.items[i].payload.packages.count;
           ++j) {
        host.host_actions.items[i].payload.packages.items[j] = nullptr;
      }
    }
  }
}

void free_config(struct config config) {
  if (config.aur_helper != nullptr) {
    free(config.aur_helper);
    config.aur_helper = nullptr;
  }
  if (config.active_host.name != nullptr) {
    free_host(config.active_host);
  }
}
