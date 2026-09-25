#include "damgr/state.h"
#include "damgr/log.h"
#include "damgr/utils.h"
#include <stdlib.h>
#include <string.h>

const char *damgr_conf_keys[] = {
    [AUR_HELPER] = "aur_helper",     [ACTIVE_HOST] = "active_host",
    [MODULES] = "modules",           [SERVICES] = "services",
    [DOTFILES] = "dotfiles",         [PACKAGES] = "packages",
    [AUR_PACKAGES] = "aur_packages", [PRE_HOOKS] = "pre_hooks",
    [POST_HOOKS] = "post_hooks"};

void damgr_darray_append(Damgr_Darray *darray, char *item) {
  if (darray->count >= darray->capacity) {
    if (darray->capacity == 0) {
      darray->capacity = 16;
    } else {
      darray->capacity *= 2;
    }
    darray->items =
        realloc(darray->items, darray->capacity * sizeof(*darray->items));
  }
  darray->items[darray->count++] = item;
}

void damgr_modules_append(Damgr_Modules *modules, Damgr_Module module) {
  if (modules->count >= modules->capacity) {
    if (modules->capacity == 0) {
      modules->capacity = 16;
    } else {
      modules->capacity *= 2;
    }
    modules->items =
        realloc(modules->items, modules->capacity * sizeof(*modules->items));
  }
  modules->items[modules->count++] = module;
}

static int damgr_parse_val(int *conf_key, Damgr_Config *config, char *line,
                           int *idx) {
  char key[256];
  char val[256];
  if (sscanf(line, "%255[^:]:%255[^\n]", key, val) == 2) {
    Damgr_Module *module = &config->active_host.modules.items[*idx];
    damgr_string_trim(key);
    damgr_string_trim(val);
    switch (*conf_key) {
    case PRE_HOOKS:
      if (memcmp(val, "true", 4) == 0) {
        damgr_darray_append(&module->pre_root_hooks, damgr_string_copy(key));
      } else {
        damgr_darray_append(&module->pre_user_hooks, damgr_string_copy(key));
      }
      break;
    case POST_HOOKS:
      if (memcmp(val, "true", 4) == 0) {
        damgr_darray_append(&module->post_root_hooks, damgr_string_copy(key));
      } else {
        damgr_darray_append(&module->post_user_hooks, damgr_string_copy(key));
      }
      break;
    case DOTFILES:
      if (memcmp(val, "true", 4) == 0) {
        module->to_link = true;
      }
      break;
    }
  }

  return EXIT_SUCCESS;
}

static int damgr_parse_line(int *conf_key, Damgr_Config *config, char *line,
                            int *idx) {
  damgr_string_trim(line);
  if (line[0] == '#' || line[0] == '\n') {
    return EXIT_SUCCESS;
  }
  char key[256];
  char val[256];
  if (damgr_string_contains(line, "=")) {
    if (sscanf(line, "%255[^=]=%255[^\n]", key, val) == 2 && *val != '\n') {
      damgr_string_trim(key);
      *conf_key = damgr_get_conf_key(key);
      damgr_string_trim(val);
      if (damgr_string_contains(val, ":")) {
        // a str = str:true, e.g. hooks one-liner
        damgr_parse_val(conf_key, config, val, idx);
      } else {
        // a str = str line, e.g. aur_helper=paru
        switch (*conf_key) {
        case AUR_HELPER:
          config->aur_helper = damgr_string_copy(val);
          break;
        case ACTIVE_HOST:
          config->active_host.name = damgr_string_copy(val);
          break;
        case MODULES:
          Damgr_Module module = {.name = damgr_string_copy(val)};
          damgr_modules_append(&config->active_host.modules, module);
          break;
        case SERVICES:
          if (idx == nullptr) {
            damgr_darray_append(&config->active_host.root_services,
                                damgr_string_copy(val));
          } else {
            damgr_darray_append(
                &config->active_host.modules.items[*idx].user_services,
                damgr_string_copy(val));
          }
          break;
        case PACKAGES:
          damgr_darray_append(&config->active_host.modules.items[*idx].packages,
                              damgr_string_copy(val));
          break;
        case AUR_PACKAGES:
          damgr_darray_append(
              &config->active_host.modules.items[*idx].aur_packages,
              damgr_string_copy(val));
          break;
        }
      }
    } else {
      *conf_key = damgr_get_conf_key(key); // also set conf key on str=\n lines
    }
  } else {
    if (damgr_string_contains(line, ":")) {
      // a __str:true line, e.g. nested hooks
      damgr_parse_val(conf_key, config, line, idx);
    } else {
      // a __str line, e.g. nested packages
      switch (*conf_key) {
      case MODULES:
        Damgr_Module module = {.name = damgr_string_copy(line)};
        damgr_modules_append(&config->active_host.modules, module);
        break;
      case SERVICES:
        if (idx == nullptr) {
          damgr_darray_append(&config->active_host.root_services,
                              damgr_string_copy(line));
        } else {
          damgr_darray_append(
              &config->active_host.modules.items[*idx].user_services,
              damgr_string_copy(line));
        }
        break;
      case PACKAGES:
        damgr_darray_append(&config->active_host.modules.items[*idx].packages,
                            damgr_string_copy(line));
        break;
      case AUR_PACKAGES:
        damgr_darray_append(
            &config->active_host.modules.items[*idx].aur_packages,
            damgr_string_copy(line));
        break;
      }
    }
  }

  return EXIT_SUCCESS;
}

static int damgr_parse_conf(FILE *fid, Damgr_Config *config, int *idx) {
  int conf_key;
  char line[512];
  size_t lines = 0;
  while (fgets(line, sizeof(line), fid)) {
    ++lines;
    if (line[0] == '#') {
      continue;
    }
    if (damgr_parse_line(&conf_key, config, line, idx) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

static int damgr_validate_config(Damgr_Config config, char *fidbuf) {
  // TODO: aur helper can be empty?
  if (config.aur_helper == nullptr) {
    damgr_log(ERROR, "failed to parse aur_helper for config: %s", fidbuf);
    return EXIT_FAILURE;
  }
  if (config.active_host.name == nullptr) {
    damgr_log(ERROR, "failed to parse host name for config: %s", fidbuf);
    return EXIT_FAILURE;
  }
  damgr_log(INFO, "successfully parsed config: %s", fidbuf);
  return EXIT_SUCCESS;
}

int damgr_read_config(char *user, Damgr_Config *config, bool is_state) {
  char fidbuf[damgr_path_max];
  if (is_state) {
    snprintf(fidbuf, sizeof(fidbuf),
             "/home/%s/.local/state/damgr/config_state.conf", user);
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damgr/config.conf",
             user);
  }
  FILE *config_fid = fopen(fidbuf, "r");
  if (config_fid != nullptr) {
    damgr_log(INFO, "parsing config: %s", fidbuf);
    if (damgr_parse_conf(config_fid, config, nullptr) != EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to parse config: %s", fidbuf);
      fclose(config_fid);
      return EXIT_FAILURE;
    }
    fclose(config_fid);
    return damgr_validate_config(*config, fidbuf);
  } else {
    char *fmt = (is_state) ? "state" : "new";
    damgr_log(ERROR, "failed to open %s config: %s", fmt, fidbuf);
    return EXIT_FAILURE;
  }
}

static int damgr_validate_host(Damgr_Host host, char *fidbuf) {
  // TODO: is there any more validation to do for the host?
  if (host.modules.count == 0) {
    damgr_log(ERROR, "no modules found for active host: %s", fidbuf);
    return EXIT_FAILURE;
  }
  damgr_log(INFO, "successfully parsed host: %s", fidbuf);
  return EXIT_SUCCESS;
}

int damgr_read_host(char *user, Damgr_Config *config, bool is_state) {
  char fidbuf[damgr_path_max];
  if (is_state) {
    snprintf(fidbuf, sizeof(fidbuf),
             "/home/%s/.local/state/damgr/%s_state.conf", user,
             config->active_host.name);
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damgr/hosts/%s.conf",
             user, config->active_host.name);
  }
  FILE *host_fid = fopen(fidbuf, "r");
  if (host_fid != nullptr) {
    damgr_log(INFO, "parsing host: %s", fidbuf);
    if (damgr_parse_conf(host_fid, config, nullptr) != EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to parse host: %s", fidbuf);
      fclose(host_fid);
      return EXIT_FAILURE;
    }
    fclose(host_fid);
    return damgr_validate_host(config->active_host, fidbuf);
  } else {
    char *fmt = (is_state) ? "state" : "new";
    damgr_log(ERROR, "failed to open %s host: %s", fmt, fidbuf);
    return EXIT_FAILURE;
  }
}

static int damgr_validate_module([[maybe_unused]] struct module module,
                                 char *fidbuf) {
  damgr_log(INFO, "successfully parsed module: %s", fidbuf);
  return EXIT_SUCCESS;
}

int damgr_read_module(char *user, Damgr_Config *config, int module_idx,
                      bool is_state) {
  struct module *module = &config->active_host.modules.items[module_idx];
  char fidbuf[damgr_path_max];
  if (is_state) {
    snprintf(fidbuf, sizeof(fidbuf),
             "/home/%s/.local/state/damgr/%s_state.conf", user, module->name);
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damgr/modules/%s.conf",
             user, module->name);
  }
  FILE *module_fid = fopen(fidbuf, "r");
  if (module_fid != nullptr) {
    damgr_log(INFO, "parsing module: %s", fidbuf);
    if (damgr_parse_conf(module_fid, config, &module_idx) != EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to parse module: %s", fidbuf);
      fclose(module_fid);
      return EXIT_FAILURE;
    }
    fclose(module_fid);
    return damgr_validate_module(*module, fidbuf);
  } else {
    char *fmt = (is_state) ? "state" : "new";
    damgr_log(ERROR, "failed to open %s module: %s", fmt, fidbuf);
    return EXIT_FAILURE;
  }
}
