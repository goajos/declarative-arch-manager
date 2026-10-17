#include "damgr/context.h"
#include "damgr/log.h"
#include "damgr/utils.h"

static int read_context(Damgr_Context *context, Damgr_Arena *arena,
                        const char *fid_buf, bool is_state) {
  FILE *context_fid = fopen(fid_buf, "r");
  if (context_fid == nullptr) {
    damgr_log(ERROR, "could not open config fid %s: %s", fid_buf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  char line[DAMGR_LINE_MAX];
  while (fgets(line, sizeof(line), context_fid)) {
    if (line[0] == '\n') // also skip newlines
      continue;
    if (line[0] == '#') // also skip comments
      continue;

    // valid config lines are all x=y
    char *eq = strchr(line, '=');
    if (!eq)
      continue;

    *eq = '\0';
    char *key = line;
    char *val = eq + 1;
    damgr_trim_string_inplace(key);
    damgr_trim_string_inplace(val);

    if (strcmp(key, "aur_helper") == 0) {
      if (!is_state) {
        // only use the active aur_helper
        context->aur_helper = damgr_arena_strdup(arena, val);
      } else {
        context->aur_helper = nullptr;
      }
    } else if (strcmp(key, "active_host") == 0) {
      if (is_state) {
        context->state_host = damgr_arena_alloc(arena, sizeof(Damgr_Host));
        if (context->state_host == nullptr) {
          damgr_log(ERROR, "arena alloc failed and returned nullptr");
          fclose(context_fid);
          return EXIT_FAILURE;
        }
        context->state_host->host_name = damgr_arena_strdup(arena, val);
        if (context->state_host->host_name == nullptr) {
          damgr_log(ERROR, "arena strdup failed and returned nullptr");
          fclose(context_fid);
          return EXIT_FAILURE;
        }
      } else {
        context->active_host = damgr_arena_alloc(arena, sizeof(Damgr_Host));
        if (context->active_host == nullptr) {
          damgr_log(ERROR, "arena alloc failed and returned nullptr");
          fclose(context_fid);
          return EXIT_FAILURE;
        }
        context->active_host->host_name = damgr_arena_strdup(arena, val);
        if (context->active_host->host_name == nullptr) {
          damgr_log(ERROR, "arena strdup failed and returned nullptr");
          fclose(context_fid);
          return EXIT_FAILURE;
        }
      }
    }
  }

  fclose(context_fid);
  return EXIT_SUCCESS;
}

static int read_host(Damgr_Host *host, Damgr_Arena *arena,
                     const char *fid_buf) {
  FILE *host_fid = fopen(fid_buf, "r");
  if (host_fid == nullptr) {
    damgr_log(ERROR, "could not open host fid %s: %s", fid_buf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  char line[DAMGR_LINE_MAX];
  char *stack_modules[DAMGR_ARRAY_BUFFER_MAX];
  size_t modules_cnt = 0;
  bool parsing = false;

  while (fgets(line, sizeof(line), host_fid)) {
    if (line[0] == '\n') // also skip newlines
      continue;
    if (line[0] == '#') // also skip comments
      continue;

    char *eq = strchr(line, '=');
    if (!eq) {
      // non = lines should be indented
      bool is_indented = isspace((unsigned char)line[0]);
      if (is_indented && parsing) {
        damgr_trim_string_inplace(line);
        if (line[0] == '\0')
          continue;
        if (line[0] == '#') // also skip indented comments
          continue;

        if (modules_cnt < DAMGR_ARRAY_BUFFER_MAX) {
          stack_modules[modules_cnt++] = damgr_arena_strdup(arena, line);
          if (stack_modules[modules_cnt - 1] == nullptr) {
            damgr_log(ERROR, "arena strdup failed and returned nullptr");
            fclose(host_fid);
            return EXIT_FAILURE;
          }
        } else {
          damgr_log(ERROR, "buffer limit %d exceeded for modules count",
                    DAMGR_ARRAY_BUFFER_MAX);
          fclose(host_fid);
          return EXIT_FAILURE;
        }
      } else {
        // non = line that is not indented
        damgr_log(ERROR,
                  "detected a not indented line while parsing modules for "
                  "host %s:\n%s",
                  host->host_name, line);
        fclose(host_fid);
        return EXIT_FAILURE;
      }
    } else {
      *eq = '\0';
      char *key = line;
      char *val = eq + 1;
      damgr_trim_string_inplace(key);
      damgr_trim_string_inplace(val);

      if (strcmp(key, "modules") == 0) {
        parsing = true;
      } else {
        damgr_log(ERROR, "detected an invalid key while parsing host %s: %s",
                  host->host_name, key);
        fclose(host_fid);
        return EXIT_FAILURE;
      }
      if (*val != '\0' && *val != '\n') {
        if (modules_cnt < DAMGR_ARRAY_BUFFER_MAX) {
          stack_modules[modules_cnt++] = damgr_arena_strdup(arena, val);
          if (stack_modules[modules_cnt - 1] == nullptr) {
            damgr_log(ERROR, "arena strdup failed and returned nullptr");
            fclose(host_fid);
            return EXIT_FAILURE;
          }
        } else {
          damgr_log(ERROR, "buffer limit %d exceeded for modules count",
                    DAMGR_ARRAY_BUFFER_MAX);
          fclose(host_fid);
          return EXIT_FAILURE;
        }
      }
    }
  }

  fclose(host_fid);

  if (modules_cnt > 0) {
    host->modules =
        damgr_arena_alloc(arena, modules_cnt * sizeof(Damgr_Module *));
    if (host->modules == nullptr) {
      damgr_log(ERROR, "arena alloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
    host->modules_cnt = modules_cnt;
    for (size_t i = 0; i < modules_cnt; ++i) {
      host->modules[i] = damgr_arena_alloc(arena, sizeof(Damgr_Module));
      if (host->modules[i] == nullptr) {
        damgr_log(ERROR, "arena alloc failed and returned nullptr");
        return EXIT_FAILURE;
      }
      host->modules[i]->module_name = stack_modules[i];
    }
  }

  return EXIT_SUCCESS;
}

static int read_module(Damgr_Module *module, Damgr_Arena *arena,
                       const char *fid_buf) {
  FILE *module_fid = fopen(fid_buf, "r");
  if (module_fid == nullptr) {
    damgr_log(ERROR, "could not open module fid %s: %s", fid_buf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  char line[DAMGR_LINE_MAX];
  enum { NONE, AUR, PKG, SVC } active_parsing = NONE;
  char *stack_packages[DAMGR_ARRAY_BUFFER_MAX];
  size_t packages_cnt = 0;
  char *stack_aur_packages[DAMGR_ARRAY_BUFFER_MAX];
  size_t aur_packages_cnt = 0;
  char *stack_root_services[DAMGR_ARRAY_BUFFER_MAX];
  size_t root_services_cnt = 0;
  char *stack_user_services[DAMGR_ARRAY_BUFFER_MAX];
  size_t user_services_cnt = 0;
  bool to_link = false;

  while (fgets(line, sizeof(line), module_fid)) {
    if (line[0] == '\n') // also skip newlines
      continue;
    if (line[0] == '#') // also skip comments
      continue;

    char *eq = strchr(line, '=');
    if (!eq) {
      // non = lines should be indented
      bool is_indented = isspace((unsigned char)line[0]);
      if (is_indented) {
        damgr_trim_string_inplace(line);
        if (line[0] == '\0')
          continue;
        if (line[0] == '#') // also skip indented comments
          continue;

        char *sep = strchr(line, ':');
        if (!sep) {
          // indented line with no = and no :
          // right now only (aur_)packages
          if (active_parsing == AUR) {
            if (aur_packages_cnt < DAMGR_ARRAY_BUFFER_MAX) {
              stack_aur_packages[aur_packages_cnt++] =
                  damgr_arena_strdup(arena, line);
              if (stack_aur_packages[aur_packages_cnt - 1] == nullptr) {
                damgr_log(ERROR, "arena strdup failed and returned nullptr");
                fclose(module_fid);
                return EXIT_FAILURE;
              }
            } else {
              damgr_log(ERROR,
                        "buffer limit %d exceeded for aur packages count",
                        DAMGR_ARRAY_BUFFER_MAX);
              fclose(module_fid);
              return EXIT_FAILURE;
            }
          } else if (active_parsing == PKG) {
            if (packages_cnt < DAMGR_ARRAY_BUFFER_MAX) {
              stack_packages[packages_cnt++] = damgr_arena_strdup(arena, line);
              if (stack_packages[packages_cnt - 1] == nullptr) {
                damgr_log(ERROR, "arena strdup failed and returned nullptr");
                fclose(module_fid);
                return EXIT_FAILURE;
              }
            } else {
              damgr_log(ERROR, "buffer limit %d exceeded for packages count",
                        DAMGR_ARRAY_BUFFER_MAX);
              fclose(module_fid);
              return EXIT_FAILURE;
            }
          } else {
            // not parsing (aur_)packages
            continue;
          }
        } else {
          // indented line with no = and :
          // right now only services
          *sep = '\0';
          char *key = line;
          char *val = sep + 1;
          damgr_trim_string_inplace(key);
          damgr_trim_string_inplace(val);
          if (active_parsing == SVC) {
            if (strcmp(val, "true") == 0) {
              if (root_services_cnt < DAMGR_ARRAY_BUFFER_MAX) {
                stack_root_services[root_services_cnt++] =
                    damgr_arena_strdup(arena, key);
                if (stack_root_services[root_services_cnt - 1] == nullptr) {
                  damgr_log(ERROR, "arena strdup failed and returned nullptr");
                  fclose(module_fid);
                  return EXIT_FAILURE;
                }
              } else {
                damgr_log(ERROR,
                          "buffer limit %d exceeded for root services count",
                          DAMGR_ARRAY_BUFFER_MAX);
                fclose(module_fid);
                return EXIT_FAILURE;
              }
            } else {
              // val == false
              if (user_services_cnt < DAMGR_ARRAY_BUFFER_MAX) {
                stack_user_services[user_services_cnt++] =
                    damgr_arena_strdup(arena, key);
                if (stack_user_services[user_services_cnt - 1] == nullptr) {
                  damgr_log(ERROR, "arena strdup failed and returned nullptr");
                  fclose(module_fid);
                  return EXIT_FAILURE;
                }
              } else {
                damgr_log(ERROR,
                          "buffer limit %d exceeded for user services count",
                          DAMGR_ARRAY_BUFFER_MAX);
                fclose(module_fid);
                return EXIT_FAILURE;
              }
            }
          } else {
            // not parsing services
            continue;
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
        if (strcmp(key1, "aur_packages") == 0) {
          active_parsing = AUR;
        } else if (strcmp(key1, "packages") == 0) {
          active_parsing = PKG;
        } else if (strcmp(key1, "services") == 0) {
          active_parsing = SVC;
        } else if (strcmp(key1, "dotfiles") == 0) {
          active_parsing = NONE;
          // TODO: this implies the dotfile link will have the module name, do
          // we want that?
          if (strcmp(val1, "true") == 0) {
            to_link = true;
          } else if (strcmp(val1, "false") == 0) {
            to_link = false;
          } else {
            damgr_log(
                ERROR,
                "detected a non boolean value for dotfiles for module  %s: %s",
                module->module_name, val1);
            fclose(module_fid);
            return EXIT_FAILURE;
          }
        } else {
          // reset active_parsing to NONE in all other cases
          active_parsing = NONE;
        }

        // val should still be parsed for (aur_)packages
        if (val1[0] == '\n')
          continue;
        if (val1[0] == '\0')
          continue;

        if (active_parsing == AUR) {
          if (aur_packages_cnt < DAMGR_ARRAY_BUFFER_MAX) {
            stack_aur_packages[aur_packages_cnt++] =
                damgr_arena_strdup(arena, line);
            if (stack_aur_packages[aur_packages_cnt - 1] == nullptr) {
              damgr_log(ERROR, "arena strdup failed and returned nullptr");
              fclose(module_fid);
              return EXIT_FAILURE;
            }
          } else {
            damgr_log(ERROR, "buffer limit %d exceeded for aur packages count",
                      DAMGR_ARRAY_BUFFER_MAX);
            fclose(module_fid);
            return EXIT_FAILURE;
          }
        } else if (active_parsing == PKG) {
          if (packages_cnt < DAMGR_ARRAY_BUFFER_MAX) {
            stack_packages[packages_cnt++] = damgr_arena_strdup(arena, line);
            if (stack_packages[packages_cnt - 1] == nullptr) {
              damgr_log(ERROR, "arena strdup failed and returned nullptr");
              fclose(module_fid);
              return EXIT_FAILURE;
            }
          } else {
            damgr_log(ERROR, "buffer limit %d exceeded for packages count",
                      DAMGR_ARRAY_BUFFER_MAX);
            fclose(module_fid);
            return EXIT_FAILURE;
          }
        } else if (active_parsing == SVC) {
          damgr_log(
              ERROR,
              "detected an invalid service line with no : for module  %s: %s",
              module->module_name, val1);
          fclose(module_fid);
          return EXIT_FAILURE;
        } else {
          // not parsing (aur_)packages
          continue;
        }

      } else {
        // line with = and :
        // right now only services
        *sep = '\0';
        char *key2 = val1;
        char *val2 = sep + 1;
        damgr_trim_string_inplace(key2);
        damgr_trim_string_inplace(val2);

        if (strcmp(key1, "services") == 0) {
          active_parsing = SVC;
        } else {
          active_parsing = NONE;
        }

        if (strcmp(val2, "true") == 0) {
          if (root_services_cnt < DAMGR_ARRAY_BUFFER_MAX) {
            stack_root_services[root_services_cnt++] =
                damgr_arena_strdup(arena, key2);
            if (stack_root_services[root_services_cnt - 1] == nullptr) {
              damgr_log(ERROR, "arena strdup failed and returned nullptr");
              fclose(module_fid);
              return EXIT_FAILURE;
            }
          } else {
            damgr_log(ERROR, "buffer limit %d exceeded for root services count",
                      DAMGR_ARRAY_BUFFER_MAX);
            fclose(module_fid);
            return EXIT_FAILURE;
          }
        } else if (strcmp(val2, "false") == 0) {
          if (user_services_cnt < DAMGR_ARRAY_BUFFER_MAX) {
            stack_user_services[user_services_cnt++] =
                damgr_arena_strdup(arena, key2);
            if (stack_user_services[user_services_cnt - 1] == nullptr) {
              damgr_log(ERROR, "arena strdup failed and returned nullptr");
              fclose(module_fid);
              return EXIT_FAILURE;
            }
          } else {
            damgr_log(ERROR, "buffer limit %d exceeded for user services count",
                      DAMGR_ARRAY_BUFFER_MAX);
            fclose(module_fid);
            return EXIT_FAILURE;
          }
        } else {
          damgr_log(
              ERROR,
              "detected a non boolean value for services for module  %s: %s",
              module->module_name, val2);
          fclose(module_fid);
          return EXIT_FAILURE;
        }
      }
    }
  }
  fclose(module_fid);

  if (packages_cnt > 0) {
    module->packages = damgr_arena_alloc(arena, packages_cnt * sizeof(char *));
    if (module->packages == nullptr) {
      damgr_log(ERROR, "arena alloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
    module->packages_cnt = packages_cnt;
    for (size_t i = 0; i < packages_cnt; ++i) {
      module->packages[i] = stack_packages[i];
    }
  }
  if (aur_packages_cnt > 0) {
    module->aur_packages =
        damgr_arena_alloc(arena, aur_packages_cnt * sizeof(char *));
    if (module->aur_packages == nullptr) {
      damgr_log(ERROR, "arena alloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
    module->aur_packages_cnt = aur_packages_cnt;
    for (size_t i = 0; i < aur_packages_cnt; ++i) {
      module->aur_packages[i] = stack_aur_packages[i];
    }
  }
  if (root_services_cnt > 0) {
    module->root_services =
        damgr_arena_alloc(arena, root_services_cnt * sizeof(char *));
    if (module->root_services == nullptr) {
      damgr_log(ERROR, "arena alloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
    module->root_services_cnt = root_services_cnt;
    for (size_t i = 0; i < root_services_cnt; ++i) {
      module->root_services[i] = stack_root_services[i];
    }
  }
  if (user_services_cnt > 0) {
    module->user_services =
        damgr_arena_alloc(arena, user_services_cnt * sizeof(char *));
    if (module->user_services == nullptr) {
      damgr_log(ERROR, "arena alloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
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

int damgr_read_state_context(Damgr_Context *context, Damgr_Arena *arena) {
  int ret;
  char fid_buf[DAMGR_FID_BUF_MAX];
  damgr_format_fid_buf(fid_buf, "config", true, NO_ACTIVE_DIR, STATE);
  ret = read_context(context, arena, fid_buf, true);
  if (ret == EXIT_FAILURE)
    return ret;

  if (context->state_host == nullptr ||
      context->state_host->host_name == nullptr) {
    damgr_log(ERROR, "failed to parse state host from state config: %s",
              fid_buf);
    return EXIT_FAILURE;
  }

  damgr_format_fid_buf(fid_buf, context->state_host->host_name, true,
                       NO_ACTIVE_DIR, STATE);
  ret = read_host(context->state_host, arena, fid_buf);
  if (ret == EXIT_FAILURE)
    return ret;

  if (context->state_host->modules_cnt == 0) {
    damgr_log(ERROR, "failed to parse any state modules from state host: %s",
              fid_buf);
    return EXIT_FAILURE;
  }

  for (size_t i = 0;
       i < context->state_host->modules_cnt && ret == EXIT_SUCCESS; ++i) {
    damgr_format_fid_buf(fid_buf, context->state_host->modules[i]->module_name,
                         true, NO_ACTIVE_DIR, STATE);
    ret = read_module(context->state_host->modules[i], arena, fid_buf);
  }

  return ret;
}

int damgr_read_active_context(Damgr_Context *context, Damgr_Arena *arena) {
  int ret;
  char fid_buf[DAMGR_FID_BUF_MAX];
  damgr_format_fid_buf(fid_buf, "config", false, CONTEXT, NO_STATE_DIR);
  ret = read_context(context, arena, fid_buf, false);
  if (ret == EXIT_FAILURE)
    return ret;

  if (context->active_host == nullptr ||
      context->active_host->host_name == nullptr) {
    damgr_log(ERROR, "failed to parse host from active config: %s", fid_buf);
    return EXIT_FAILURE;
  }

  if (context->aur_helper == nullptr) {
    damgr_log(ERROR, "failed to parse aur_helper from active config: %s",
              fid_buf);
    return EXIT_FAILURE;
  }

  damgr_format_fid_buf(fid_buf, context->active_host->host_name, false, HOST,
                       NO_STATE_DIR);
  ret = read_host(context->active_host, arena, fid_buf);
  if (ret == EXIT_FAILURE)
    return ret;

  if (context->active_host->modules_cnt == 0) {
    damgr_log(ERROR, "failed to parse any modules from active host: %s",
              fid_buf);
    return EXIT_FAILURE;
  }

  for (size_t i = 0;
       i < context->active_host->modules_cnt && ret == EXIT_SUCCESS; ++i) {
    snprintf(fid_buf, sizeof(fid_buf), "%s/.context/damgr/modules/%s.conf",
             getenv("HOME"), context->active_host->modules[i]->module_name);
    damgr_format_fid_buf(fid_buf, context->active_host->modules[i]->module_name,
                         false, MODULE, NO_STATE_DIR);
    ret = read_module(context->active_host->modules[i], arena, fid_buf);
  }

  return ret;
}
