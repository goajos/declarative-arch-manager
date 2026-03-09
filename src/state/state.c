#include "damgr/state.h"
#include "damgr/log.h"
#include "damgr/utils.h"
#include <stdlib.h>
// #include <string.h>

const char *damgr_conf_keys[] = {
    [AUR_HELPER] = "aur_helper", [ACTIVE_HOST] = "active_host",
    [MODULES] = "modules",       [SERVICES] = "services",
    [DOTFILES] = "dotfiles",     [PACKAGES] = "packages",
    [PRE_HOOKS] = "pre_hooks",   [POST_HOOKS] = "post_hooks"};

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

int damgr_parse_line(int *conf_key, Damgr_Config *config, char *line) {
  if (damgr_string_contains(line, "=")) {
    char key[256];
    char val[256];
    int ret = sscanf(line, "%255[^=]=%255[^\n]", key, val);
    if (ret != 1 && ret != 2) {
      damgr_log(ERROR, "sscanf failed for line: %s", line);
      return EXIT_FAILURE;
    }
    damgr_string_trim(key);
    *conf_key = damgr_get_conf_key(key);
    damgr_string_trim(val);
    if (val[0] != '\0') {
      switch (*conf_key) {
      case AUR_HELPER:
        config->aur_helper = damgr_string_copy(val);
        break;
      case ACTIVE_HOST:
        config->active_host.name = damgr_string_copy(val);
        break;
      }
    }
  } else {
    damgr_string_trim(line);
    if (line[0] == '#' || line[0] == '\n') {
      return EXIT_SUCCESS;
    }
    switch (*conf_key) {
    case MODULES:
      Damgr_Module module = {.name = damgr_string_copy(line)};
      damgr_modules_append(&config->active_host.modules, module);
      break;
    case SERVICES:
      damgr_darray_append(&config->active_host.root_services,
                          damgr_string_copy(line));
      break;
    }
  }
  return EXIT_SUCCESS;
}

int damgr_parse_conf(FILE *fid, Damgr_Config *config) {
  int conf_key;
  char line[512];
  size_t lines = 0;
  while (fgets(line, sizeof(line), fid)) {
    ++lines;
    if (line[0] == '#') {
      continue;
    }
    if (damgr_parse_line(&conf_key, config, line) != EXIT_SUCCESS) {
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
    if (damgr_parse_conf(config_fid, config) != EXIT_SUCCESS) {
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
    if (damgr_parse_conf(host_fid, config) != EXIT_SUCCESS) {
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
