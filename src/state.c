#include "damgr/state.h"
#include "damgr/log.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

void arena_init(Damgr_Arena *arena) {
  if (arena != nullptr)
    arena->offset = 0;
}

void *arena_alloc(Damgr_Arena *arena, size_t size) {
  if (arena == nullptr || size == 0)
    return nullptr;

  size = (size + DAMGR_ALLOC_UNIT - 1) & ~(DAMGR_ALLOC_UNIT - 1);
  if (arena->offset + size > DAMGR_BUFFER_SIZE) {
    return nullptr; // out of memory
  } else {
    void *ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    memset(ptr, 0, size); // zero memory
    return ptr;
  }
}

char *arena_strdup(Damgr_Arena *arena, const char *src) {
  if (arena == nullptr || src == nullptr)
    return nullptr;

  size_t len = strlen(src) + 1; // null terminator
  char *dest = arena_alloc(arena, len);
  if (dest != nullptr) {
    memcpy(dest, src, len);
  }

  return dest;
}

static void damgr_trim_string_inplace(char *str) {
  if (*str == '\0' || *str == '\n')
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

int damgr_is_state_dir_empty(const char *dir) {
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

static int damgr_read_config(Damgr_Config *config, Damgr_Arena *arena,
                             const char *fidbuf, bool is_state) {
  FILE *config_fid = fopen(fidbuf, "r");
  if (config_fid == nullptr) {
    damgr_log(ERROR, "could not open config fid %s: %s", fidbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  char line[256];
  while (fgets(line, sizeof(line), config_fid)) {
    if (line[0] == '#') // also skip comments
      continue;

    char *eq = strchr(line, '=');
    if (!eq)
      continue;

    *eq = '\0';
    char *key = line;
    char *val = eq + 1;
    damgr_trim_string_inplace(key);
    damgr_trim_string_inplace(val);

    if (strcmp(key, "aur_helper") == 0) {
      config->aur_helper = arena_strdup(arena, val);
    } else if (strcmp(key, "active_host") == 0) {
      if (is_state) {
        config->state_host = arena_alloc(arena, sizeof(Damgr_Host));
        config->state_host->host_name = arena_strdup(arena, val);
      } else {
        config->active_host = arena_alloc(arena, sizeof(Damgr_Host));
        config->active_host->host_name = arena_strdup(arena, val);
      }
    }
  }

  fclose(config_fid);
  return EXIT_SUCCESS;
}

static int damgr_read_host(Damgr_Host *host, Damgr_Arena *arena,
                           const char *fidbuf) {
  FILE *host_fid = fopen(fidbuf, "r");
  if (host_fid == nullptr) {
    damgr_log(ERROR, "could not open host fid %s: %s", fidbuf, strerror(errno));
    return EXIT_FAILURE;
  }

  char line[256];
  char *stack_modules[DAMGR_LIST_BUFFER];
  size_t modules_cnt = 0;
  bool parsing = false;

  while (fgets(line, sizeof(line), host_fid)) {
    if (line[0] == '#') // also skip comments
      continue;

    char *eq = strchr(line, '=');
    if (!eq) {
      bool is_indented = isspace((unsigned char)line[0]);
      damgr_trim_string_inplace(line);
      if (line[0] == '\0')
        continue;
      if (line[0] == '#') // also skip comments
        continue;

      if (is_indented && parsing && modules_cnt < DAMGR_LIST_BUFFER) {
        stack_modules[modules_cnt++] = arena_strdup(arena, line);
      }
    } else {
      *eq = '\0';
      char *key = line;
      char *val = eq + 1;
      damgr_trim_string_inplace(key);
      damgr_trim_string_inplace(val);

      if (strcmp(key, "modules") == 0) {
        parsing = true;
      }
      if (*val != '\0' && *val != '\n') {
        if (modules_cnt < DAMGR_LIST_BUFFER) {
          stack_modules[modules_cnt++] = arena_strdup(arena, val);
        }
      }
    }
  }

  fclose(host_fid);

  if (modules_cnt > 0) {
    host->modules = arena_alloc(arena, modules_cnt * sizeof(Damgr_Module));
    host->modules_cnt = modules_cnt;
    for (size_t i = 0; i < modules_cnt; ++i) {
      host->modules[i] = arena_alloc(arena, sizeof(Damgr_Module));
      host->modules[i]->module_name = stack_modules[i];
    }
  }

  return EXIT_SUCCESS;
}

static int damgr_read_module(Damgr_Module *module, Damgr_Arena *arena,
                             const char *fidbuf) {
  FILE *module_fid = fopen(fidbuf, "r");
  if (module_fid == nullptr) {
    damgr_log(ERROR, "could not open module fid %s: %s", fidbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  char line[256];
  enum { NONE, AUR, PKG, SERVICE } active_parsing = NONE;
  char *stack_packages[DAMGR_LIST_BUFFER];
  size_t packages_cnt = 0;
  char *stack_aur_packages[DAMGR_LIST_BUFFER];
  size_t aur_packages_cnt = 0;
  char *stack_root_services[DAMGR_LIST_BUFFER];
  size_t root_services_cnt = 0;
  char *stack_user_services[DAMGR_LIST_BUFFER];
  size_t user_services_cnt = 0;
  bool to_link = false;

  while (fgets(line, sizeof(line), module_fid)) {
    if (line[0] == '#') // also skip comments
      continue;

    char *eq = strchr(line, '=');
    if (!eq) {
      // no = in line
      bool is_indented = isspace((unsigned char)line[0]);
      damgr_trim_string_inplace(line);
      if (line[0] == '\0')
        continue;
      if (line[0] == '#') // also skip comments
        continue;

      if (is_indented) {
        char *sep = strchr(line, ':');
        if (!sep) {
          // indented line with no = and no :
          // right now only (aur_)packages
          if (active_parsing == AUR && aur_packages_cnt < DAMGR_LIST_BUFFER) {
            stack_aur_packages[aur_packages_cnt++] = arena_strdup(arena, line);
          } else if (active_parsing == PKG &&
                     packages_cnt < DAMGR_LIST_BUFFER) {
            stack_packages[packages_cnt++] = arena_strdup(arena, line);
          }
        } else {
          // indented line with no = and :
          // right now only services
          *sep = '\0';
          char *key = line;
          char *val = sep + 1;
          if (active_parsing == SERVICE) {
            if (strcmp(val, "true") == 0) {
              if (root_services_cnt < DAMGR_LIST_BUFFER) {
                stack_root_services[root_services_cnt++] =
                    arena_strdup(arena, key);
              }
            } else {
              if (user_services_cnt < DAMGR_LIST_BUFFER) {
                stack_user_services[user_services_cnt++] =
                    arena_strdup(arena, key);
              }
            }
          }
        }
      }
    } else {
      // = in line have to set active_parsing!
      *eq = '\0';
      char *key1 = line;
      char *val1 = eq + 1;
      damgr_trim_string_inplace(key1);
      damgr_trim_string_inplace(val1);
      char *sep = strchr(val1, ':');
      if (!sep) {
        // line with = and no :
        // right now only (aur_)packages
        if (strcmp(key1, "aur_packages") == 0) {
          active_parsing = AUR;
        } else if (strcmp(key1, "packages") == 0) {
          active_parsing = PKG;
        } else if (strcmp(key1, "services") == 0) {
          active_parsing = SERVICE;
        } else {
          active_parsing = NONE;
        }
        // val can still be a (aur_)package
        if (*val1 != '\0' && *val1 != '\n') {
          if (active_parsing == AUR && aur_packages_cnt < DAMGR_LIST_BUFFER) {
            stack_aur_packages[aur_packages_cnt++] = arena_strdup(arena, val1);
          } else if (active_parsing == PKG &&
                     packages_cnt < DAMGR_LIST_BUFFER) {
            stack_packages[packages_cnt++] = arena_strdup(arena, val1);
          }
        }
      } else {
        // line with = and :
        // right now services and dotfiles
        *sep = '\0';
        char *key2 = val1;
        char *val2 = sep + 1;
        if (strcmp(key1, "dotfiles") == 0) {
          if (strcmp(val2, "true") == 0) {
            to_link = true;
          }
        } else if (strcmp(key1, "services") == 0) {
          active_parsing = SERVICE;
          if (strcmp(val2, "true") == 0) {
            if (root_services_cnt < DAMGR_LIST_BUFFER) {
              stack_root_services[root_services_cnt++] =
                  arena_strdup(arena, key2);
            }
          } else {
            if (user_services_cnt < DAMGR_LIST_BUFFER) {
              stack_user_services[user_services_cnt++] =
                  arena_strdup(arena, key2);
            }
          }
        }
      }
    }
  }
  fclose(module_fid);

  if (packages_cnt > 0) {
    module->packages = arena_alloc(arena, packages_cnt * sizeof(char *));
    module->packages_cnt = packages_cnt;
    for (size_t i = 0; i < packages_cnt; ++i) {
      module->packages[i] = stack_packages[i];
    }
  }
  if (aur_packages_cnt > 0) {
    module->aur_packages =
        arena_alloc(arena, aur_packages_cnt * sizeof(char *));
    module->aur_packages_cnt = aur_packages_cnt;
    for (size_t i = 0; i < aur_packages_cnt; ++i) {
      module->aur_packages[i] = stack_aur_packages[i];
    }
  }
  if (root_services_cnt > 0) {
    module->root_services =
        arena_alloc(arena, root_services_cnt * sizeof(char *));
    module->root_services_cnt = root_services_cnt;
    for (size_t i = 0; i < root_services_cnt; ++i) {
      module->root_services[i] = stack_root_services[i];
    }
  }
  if (user_services_cnt > 0) {
    module->user_services =
        arena_alloc(arena, user_services_cnt * sizeof(char *));
    module->user_services_cnt = user_services_cnt;
    for (size_t i = 0; i < user_services_cnt; ++i) {
      module->user_services[i] = stack_user_services[i];
    }
  }
  if (to_link) {
    module->to_link = true;
  }

  return EXIT_SUCCESS;
}

int damgr_read_state(Damgr_Config *config, Damgr_Arena *arena) {
  int ret;
  char fidbuf[DAMGR_PATH_MAX];
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/config_state.conf",
           getenv("HOME"));
  ret = damgr_read_config(config, arena, fidbuf, true);
  if (ret == EXIT_FAILURE)
    return ret;

  if (config->state_host == nullptr ||
      config->state_host->host_name == nullptr) {
    damgr_log(ERROR, "failed to parse state host from state config: %s",
              fidbuf);
    return EXIT_FAILURE;
  }

  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/%s_state.conf",
           getenv("HOME"), config->state_host->host_name);
  ret = damgr_read_host(config->state_host, arena, fidbuf);
  if (ret == EXIT_FAILURE)
    return ret;

  if (config->state_host->modules_cnt == 0) {
    damgr_log(ERROR, "failed to parse any state modules from state host: %s",
              fidbuf);
    return EXIT_FAILURE;
  }

  for (size_t i = 0; i < config->state_host->modules_cnt; ++i) {
    snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/%s_state.conf",
             getenv("HOME"), config->state_host->modules[i]->module_name);
    ret = damgr_read_module(config->state_host->modules[i], arena, fidbuf);
    if (ret == EXIT_FAILURE)
      return ret;
  }

  return EXIT_SUCCESS;
}

int damgr_read(Damgr_Config *config, Damgr_Arena *arena) {
  int ret;
  char fidbuf[DAMGR_PATH_MAX];
  snprintf(fidbuf, sizeof(fidbuf), "%s/.config/damgr/config.conf",
           getenv("HOME"));
  ret = damgr_read_config(config, arena, fidbuf, false);
  if (ret == EXIT_FAILURE)
    return ret;

  if (config->active_host == nullptr ||
      config->active_host->host_name == nullptr) {
    damgr_log(ERROR, "failed to parse host from config: %s", fidbuf);
    return EXIT_FAILURE;
  }

  snprintf(fidbuf, sizeof(fidbuf), "%s/.config/damgr/hosts/%s.conf",
           getenv("HOME"), config->active_host->host_name);
  ret = damgr_read_host(config->active_host, arena, fidbuf);
  if (ret == EXIT_FAILURE)
    return ret;

  if (config->active_host->modules_cnt == 0) {
    damgr_log(ERROR, "failed to parse any modules from active host: %s",
              fidbuf);
    return EXIT_FAILURE;
  }

  for (size_t i = 0; i < config->active_host->modules_cnt; ++i) {
    snprintf(fidbuf, sizeof(fidbuf), "%s/.config/damgr/modules/%s.conf",
             getenv("HOME"), config->active_host->modules[i]->module_name);
    ret = damgr_read_module(config->active_host->modules[i], arena, fidbuf);
    if (ret == EXIT_FAILURE)
      return ret;
  }

  return EXIT_SUCCESS;
}
