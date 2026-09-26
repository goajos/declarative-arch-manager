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

void damgr_root_services_append(Damgr_Root_Services *services, char *service) {
  if (services->count >= services->capacity) {
    if (services->capacity == 0) {
      services->capacity = 16;
    } else {
      services->capacity *= 2;
    }
    services->items =
        realloc(services->items, services->capacity * sizeof(*services->items));
  }
  services->items[services->count++] = service;
}

static int damgr_parse_val(int *conf_key, Damgr_Config *config, char *line,
                           int *idx) {
  char key[256];
  char val[256];
  int ret = sscanf(line, "%255[^:]:%255[^\n]", key, val);
  if (ret != 1 && ret != 2) {
    damgr_log(ERROR, "failed to parse key:value from line: %s", line);
    return EXIT_FAILURE;
  } else {
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
    default:
      damgr_log(ERROR, "failed to parse line: %s", line);
      return EXIT_FAILURE;
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
    int ret = sscanf(line, "%255[^=]=%255[^\n]", key, val);
    if (ret != 1 && ret != 2) {
      damgr_log(ERROR, "failed to parse key:value from line: %s", line);
      return EXIT_FAILURE;
    }
    // make sure the conf key is always set on = lines
    damgr_string_trim(key);
    *conf_key = damgr_get_conf_key(key);
    if (ret == 2 && *val != '\n') {
      damgr_string_trim(val);
      if (damgr_string_contains(val, ":")) {
        // a str = str:true, e.g. hooks one-liner
        if (damgr_parse_val(conf_key, config, val, idx) != EXIT_SUCCESS) {
          return EXIT_FAILURE;
        }
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
          Damgr_Module module = {.name = damgr_string_copy(val),
                                 .is_compared = false,
                                 .is_orphan = true};
          damgr_modules_append(&config->active_host.modules, module);
          break;
        case SERVICES:
          if (idx == nullptr) {
            damgr_root_services_append(&config->active_host.root_services,
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
        default:
          damgr_log(ERROR, "failed to parse line: %s", line);
          return EXIT_FAILURE;
        }
      }
    }
  } else {
    if (damgr_string_contains(line, ":")) {
      // a __str:true line, e.g. nested hooks
      if (damgr_parse_val(conf_key, config, line, idx) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
    } else {
      // a __str line, e.g. nested packages
      switch (*conf_key) {
      case MODULES:
        Damgr_Module module = {.name = damgr_string_copy(line),
                               .is_compared = false,
                               .is_orphan = true};
        damgr_modules_append(&config->active_host.modules, module);
        break;
      case SERVICES:
        if (idx == nullptr) { // not parsing a module
          damgr_root_services_append(&config->active_host.root_services,
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
      default:
        damgr_log(ERROR, "failed to parse line: %s", line);
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

static int damgr_parse_conf(FILE *fid, Damgr_Config *config, int *idx) {
  int conf_key;
  char line[512];
  while (fgets(line, sizeof(line), fid)) {
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

// TODO: add a return status with log?
void damgr_remove_host(char *user, Damgr_Host host) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr/%s_state.conf",
           user, host.name);
  remove(fidbuf);
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

// TODO: add a return status with log?
void damgr_remove_module(char *user, Damgr_Module module) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr/%s_state.conf",
           user, module.name);
  remove(fidbuf);
}

static int damgr_write_module(char *user, Damgr_Module module) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr/%s_state.conf",
           user, module.name);
  FILE *module_fid = fopen(fidbuf, "w");
  if (module_fid == nullptr) {
    damgr_log(ERROR, "failed to open state module for writing: %s", fidbuf);
    return EXIT_FAILURE;
  }

  if (module.to_link) {
    fprintf(module_fid, "%s=link:true\n", damgr_conf_keys[DOTFILES]);
  }

  if (module.pre_root_hooks.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[PRE_HOOKS]);
    for (size_t i = 0; i < module.pre_root_hooks.count; ++i) {
      fprintf(module_fid, "  %s:true\n", module.pre_root_hooks.items[i]);
    }
  }
  if (module.pre_user_hooks.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[PRE_HOOKS]);
    for (size_t i = 0; i < module.pre_user_hooks.count; ++i) {
      fprintf(module_fid, "  %s:false\n", module.pre_user_hooks.items[i]);
    }
  }

  if (module.packages.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[PACKAGES]);
    for (size_t i = 0; i < module.packages.count; ++i) {
      fprintf(module_fid, "  %s\n", module.packages.items[i]);
    }
  }

  if (module.aur_packages.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[AUR_PACKAGES]);
    for (size_t i = 0; i < module.aur_packages.count; ++i) {
      fprintf(module_fid, "  %s\n", module.aur_packages.items[i]);
    }
  }

  if (module.user_services.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[SERVICES]);
    for (size_t i = 0; i < module.user_services.count; ++i) {
      fprintf(module_fid, "  %s\n", module.user_services.items[i]);
    }
  }

  if (module.post_root_hooks.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[POST_HOOKS]);
    for (size_t i = 0; i < module.post_root_hooks.count; ++i) {
      fprintf(module_fid, "  %s:true\n", module.post_root_hooks.items[i]);
    }
  }
  if (module.post_user_hooks.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[POST_HOOKS]);
    for (size_t i = 0; i < module.post_user_hooks.count; ++i) {
      fprintf(module_fid, "  %s:false\n", module.post_user_hooks.items[i]);
    }
  }

  fclose(module_fid);
  damgr_log(INFO, "succesfully wrote state module: %s", fidbuf);
  return EXIT_SUCCESS;
}

static int damgr_write_host(char *user, Damgr_Host host) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr/%s_state.conf",
           user, host.name);
  FILE *host_fid = fopen(fidbuf, "w");
  if (host_fid == nullptr) {
    damgr_log(ERROR, "failed to open state host for writing: %s", fidbuf);
    return EXIT_FAILURE;
  }

  if (host.modules.count > 0) {
    fprintf(host_fid, "%s=\n", damgr_conf_keys[MODULES]);
    for (size_t i = 0; i < host.modules.count; ++i) {
      if (host.modules.items[i].to_write) {
        fprintf(host_fid, "  %s\n", host.modules.items[i].name);
        // nested write module call
        damgr_write_module(user, host.modules.items[i]);
      }
    }
  }

  if (host.root_services.count > 0) {
    if (host.root_services.to_write) {
      fprintf(host_fid, "%s=\n", damgr_conf_keys[SERVICES]);
      for (size_t i = 0; i < host.root_services.count; ++i) {
        fprintf(host_fid, "  %s\n", host.root_services.items[i]);
      }
    }
  }

  fclose(host_fid);
  damgr_log(INFO, "succesfully wrote state host: %s", fidbuf);
  return EXIT_SUCCESS;
}

int damgr_write_config(char *user, Damgr_Config config) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf),
           "/home/%s/.local/state/damgr/config_state.conf", user);
  FILE *config_fid = fopen(fidbuf, "w");
  if (config_fid == nullptr) {
    damgr_log(ERROR, "failed to open state config for writing: %s", fidbuf);
    return EXIT_FAILURE;
  }

  if (config.aur_helper != nullptr) {
    fprintf(config_fid, "%s=%s\n", damgr_conf_keys[AUR_HELPER],
            config.aur_helper);
  }
  if (config.active_host.name != nullptr) {
    fprintf(config_fid, "%s=%s\n", damgr_conf_keys[ACTIVE_HOST],
            config.active_host.name);
  }

  fclose(config_fid);
  damgr_log(INFO, "succesfully wrote state config: %s", fidbuf);

  // nested write host call
  damgr_write_host(user, config.active_host);

  return EXIT_SUCCESS;
}

void damgr_free_darray(Damgr_Darray *darray) {
  for (size_t i = 0; i < darray->count; ++i) {
    free(darray->items[i]);
  }
  free(darray->items); // free items buffer itself
}

static void damgr_free_module(Damgr_Module *module) {
  damgr_free_darray(&module->pre_root_hooks);
  damgr_free_darray(&module->pre_user_hooks);
  damgr_free_darray(&module->packages);
  damgr_free_darray(&module->aur_packages);
  damgr_free_darray(&module->user_services);
  damgr_free_darray(&module->post_root_hooks);
  damgr_free_darray(&module->post_user_hooks);
  free(module->name);
}

void damgr_free_config(Damgr_Config *config) {
  for (size_t i = 0; i < config->active_host.modules.count; ++i) {
    damgr_free_module(&config->active_host.modules.items[i]);
  }
  free(config->active_host.modules.items); // free modules buffer itself
  for (size_t i = 0; i < config->active_host.root_services.count; ++i) {
    free(config->active_host.root_services.items[i]);
  }
  free(config->active_host.root_services
           .items); // free root services buffer itself
  free(config->active_host.name);
  free(config->aur_helper);
}
