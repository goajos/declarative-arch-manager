#include "damgr/state.h"
#include "damgr/log.h"
#include "damgr/utils.h"
#include <stdlib.h>
#include <string.h>

// TODO: can there be a single hooks to make cases better?
const char *damgr_conf_keys[] = {
    [AUR_HELPER] = "aur_helper",     [ACTIVE_HOST] = "active_host",
    [MODULES] = "modules",           [SERVICES] = "services",
    [DOTFILES] = "dotfiles",         [PACKAGES] = "packages",
    [AUR_PACKAGES] = "aur_packages", [PRE_HOOKS] = "pre_hooks",
    [POST_HOOKS] = "post_hooks"};

void damgr_modules_append(Damgr_Modules *modules, Damgr_Module module) {
  if (modules->count >= modules->capacity) {
    if (modules->capacity == 0) {
      modules->capacity = 16;
    } else {
      modules->capacity *= 2;
    }
    modules->modules = realloc(modules->modules,
                               modules->capacity * sizeof(*modules->modules));
  }
  modules->modules[modules->count++] = module;
}

static void damgr_parse_val(int *conf_key, Damgr_Config *config, char *line,
                            int idx) {
  char key[256];
  char val[256];
  int ret = sscanf(line, "%255[^:]:%255[^\n]", key, val);
  if (ret != 1 && ret != 2) {
    goto exit;
  } else {
    Damgr_Module *module = &config->active_host.modules.modules[idx];
    damgr_string_trim(key);
    damgr_string_trim(val);
    switch (*conf_key) {
    case PRE_HOOKS:
      if (memcmp(val, "true", 4) == 0) {
        damgr_darray_append(&module->pre_root_hooks, strdup(key));
      } else {
        damgr_darray_append(&module->pre_user_hooks, strdup(key));
      }
      break;
    case POST_HOOKS:
      if (memcmp(val, "true", 4) == 0) {
        damgr_darray_append(&module->post_root_hooks, strdup(key));
      } else {
        damgr_darray_append(&module->post_user_hooks, strdup(key));
      }
      break;
    case SERVICES:
      if (memcmp(val, "true", 4) == 0) {
        damgr_darray_append(&module->root_services, strdup(key));
      } else {
        damgr_darray_append(&module->user_services, strdup(key));
      }
      break;
    case DOTFILES:
      if (memcmp(val, "true", 4) == 0) {
        module->to_link = true;
      }
      break;
    default:
      goto exit;
    }
  }
  return;
exit:
  damgr_log(ERROR,
            "failed to parse key:value from val: %s for host %s in module %s",
            line, config->active_host.host_name,
            config->active_host.modules.modules[idx].module_name);
  exit(EXIT_FAILURE);
}

static void damgr_parse_line(int *conf_key, Damgr_Config *config, char *line,
                             int idx) {
  damgr_string_trim(line);
  if (line[0] == '#' || line[0] == '\n') {
    return; // nothing to parse
  }
  char key[256];
  char val[256];
  if (damgr_string_contains(line, "=")) {
    int ret = sscanf(line, "%255[^=]=%255[^\n]", key, val);
    if (ret != 1 && ret != 2) {
      goto exit;
    }
    // make sure the conf key is always set on = lines
    damgr_string_trim(key);
    *conf_key = damgr_get_conf_key(key);
    if (ret == 2 && *val != '\n') {
      damgr_string_trim(val);
      if (damgr_string_contains(val, ":")) {
        // a str = str:true, e.g. hooks one-liner
        damgr_parse_val(conf_key, config, val, idx);
      } else {
        // a str = str line, e.g. aur_helper=paru
        switch (*conf_key) {
        case AUR_HELPER:
          config->aur_helper = strdup(val);
          break;
        case ACTIVE_HOST:
          config->active_host.host_name = strdup(val);
          break;
        case MODULES:
          Damgr_Module module = {.module_name = strdup(val)};
          damgr_modules_append(&config->active_host.modules, module);
          break;
        case PACKAGES:
          damgr_darray_append(
              &config->active_host.modules.modules[idx].packages, strdup(val));
          break;
        case AUR_PACKAGES:
          damgr_darray_append(
              &config->active_host.modules.modules[idx].aur_packages,
              strdup(val));
          break;
        default:
          goto exit;
        }
      }
    }
  } else {
    if (damgr_string_contains(line, ":")) {
      // a __str:true line, e.g. nested hooks
      damgr_parse_val(conf_key, config, line, idx);
    } else {
      // a __str line, e.g. nested packages
      switch (*conf_key) {
      case MODULES:
        Damgr_Module module = {.module_name = strdup(line)};
        damgr_modules_append(&config->active_host.modules, module);
        break;
      case PACKAGES:
        damgr_darray_append(&config->active_host.modules.modules[idx].packages,
                            strdup(line));
        break;
      case AUR_PACKAGES:
        damgr_darray_append(
            &config->active_host.modules.modules[idx].aur_packages,
            strdup(line));
        break;
      default:
        goto exit;
      }
    }
  }
  return;
exit:
  if (idx == -1) {
    if (config->active_host.host_name != nullptr) {
      damgr_log(ERROR, "failed to parse key:value from line: %s for host %s",
                line, config->active_host.host_name);
    } else {
      damgr_log(ERROR, "failed to parse key:value from line: %s for config",
                line, config->active_host.host_name);
    }
  } else {
    damgr_log(
        ERROR,
        "failed to parse key:value from line: %s for host %s in module %s",
        line, config->active_host.host_name,
        config->active_host.modules.modules[idx].module_name);
  }
  exit(EXIT_FAILURE);
}

static void damgr_parse_conf(FILE *fid, Damgr_Config *config, int idx) {
  int conf_key;
  char line[512];
  while (fgets(line, sizeof(line), fid)) {
    if (line[0] == '#') {
      continue;
    }
    damgr_parse_line(&conf_key, config, line, idx);
  }
}

void damgr_read_config(Damgr_Config *config, bool is_state) {
  char fidbuf[damgr_path_max];
  if (is_state) {
    snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/config_state.conf",
             getenv("HOME"));
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "%s/.config/damgr/config.conf",
             getenv("HOME"));
  }
  FILE *config_fid = fopen(fidbuf, "r");
  damgr_log(INFO, "parsing config: %s", fidbuf);
  damgr_parse_conf(config_fid, config, -1);
  fclose(config_fid);
}

void damgr_read_host(Damgr_Config *config, bool is_state) {
  char fidbuf[damgr_path_max];
  if (is_state) {
    snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/%s_state.conf",
             getenv("HOME"), config->active_host.host_name);
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "%s/.config/damgr/hosts/%s.conf",
             getenv("HOME"), config->active_host.host_name);
  }
  FILE *host_fid = fopen(fidbuf, "r");
  damgr_log(INFO, "parsing host: %s", fidbuf);
  damgr_parse_conf(host_fid, config, -1);
  fclose(host_fid);
}

void damgr_read_module(Damgr_Config *config, int module_idx, bool is_state) {
  Damgr_Module module = config->active_host.modules.modules[module_idx];
  char fidbuf[damgr_path_max];
  if (is_state) {
    snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/%s_state.conf",
             getenv("HOME"), module.module_name);
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "%s/.config/damgr/modules/%s.conf",
             getenv("HOME"), module.module_name);
  }
  FILE *module_fid = fopen(fidbuf, "r");
  damgr_log(INFO, "parsing module: %s", fidbuf);
  damgr_parse_conf(module_fid, config, module_idx);
  fclose(module_fid);
}

void damgr_remove_module(Damgr_Module module) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/%s_state.conf",
           getenv("HOME"), module.module_name);
  if (remove(fidbuf) == EXIT_SUCCESS) {
    damgr_log(INFO, "succesfully removed state module: %s", fidbuf);
  }
}

void damgr_write_module(Damgr_Module module) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/%s_state.conf",
           getenv("HOME"), module.module_name);
  FILE *module_fid = fopen(fidbuf, "w");

  if (module.to_link) {
    fprintf(module_fid, "%s=link:true\n", damgr_conf_keys[DOTFILES]);
  }

  if (module.pre_root_hooks.count > 0 || module.pre_user_hooks.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[PRE_HOOKS]);
    for (size_t i = 0; i < module.pre_root_hooks.count; ++i) {
      fprintf(module_fid, "  %s:true\n", module.pre_root_hooks.items[i]);
    }
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

  if (module.root_services.count > 0 || module.user_services.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[SERVICES]);
    for (size_t i = 0; i < module.root_services.count; ++i) {
      fprintf(module_fid, "  %s\n", module.root_services.items[i]);
    }
    for (size_t i = 0; i < module.user_services.count; ++i) {
      fprintf(module_fid, "  %s\n", module.user_services.items[i]);
    }
  }

  if (module.post_root_hooks.count > 0 || module.post_user_hooks.count > 0) {
    fprintf(module_fid, "%s=\n", damgr_conf_keys[POST_HOOKS]);
    for (size_t i = 0; i < module.post_root_hooks.count; ++i) {
      fprintf(module_fid, "  %s:true\n", module.post_root_hooks.items[i]);
    }
    for (size_t i = 0; i < module.post_user_hooks.count; ++i) {
      fprintf(module_fid, "  %s:false\n", module.post_user_hooks.items[i]);
    }
  }

  fclose(module_fid);
  damgr_log(INFO, "succesfully wrote state module: %s", fidbuf);
}

void damgr_remove_host(Damgr_Host host) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/%s_state.conf",
           getenv("HOME"), host.host_name);
  if (remove(fidbuf) == EXIT_SUCCESS) {
    damgr_log(INFO, "succesfully removed state host: %s", fidbuf);
  }
}

void damgr_write_host(Damgr_Host host) {
  char fidbuf[damgr_path_max];
  snprintf(fidbuf, sizeof(fidbuf), "%s/.local/state/damgr/%s_state.conf",
           getenv("HOME"), host.host_name);
  FILE *host_fid = fopen(fidbuf, "w");
  if (host.modules.count > 0) {
    fprintf(host_fid, "%s=\n", damgr_conf_keys[MODULES]);
    for (size_t i = 0; i < host.modules.count; ++i) {
      // module_name == nullptr if the task queue transaction for this module
      // failed
      if (host.modules.modules[i].module_name != nullptr) {
        fprintf(host_fid, "  %s\n", host.modules.modules[i].module_name);
      }
    }
  }
  fclose(host_fid);
  damgr_log(INFO, "succesfully wrote state host: %s", fidbuf);
}
