#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include "kdl/kdl.h"
#include <stdlib.h>
#include <string.h>

static int validate_module([[maybe_unused]] struct module module,
                           char *fidbuf) {
  // TODO: is there any validation to do for the module?
  LOG(LOG_INFO, "successfully parsed module: %s", fidbuf);
  return EXIT_SUCCESS;
}

int parse_module(FILE *fid, struct module *module) {
  kdl_parser *parser =
      kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS);

  size_t depth = 0;
  char *node_d2 = nullptr;
  char *node_d3 = nullptr;
  bool eof = false;

  while (true) {
    kdl_event_data *next_event = kdl_parser_next_event(parser);
    kdl_event event = next_event->event;
    const char *name_data = next_event->name.data;
    kdl_value value = next_event->value;
    switch (event) {
    case KDL_EVENT_START_NODE:
      switch (depth) {
      case 0: // module(_state) level
        // reading module
        if (strlen(name_data) == 6 && memcmp(name_data, "module", 6) != 0) {
          return EXIT_FAILURE; // module kdl should start with module {...}
        }
        // reading state module
        if (strlen(name_data) == 12 &&
            memcmp(name_data, "module_state", 12) != 0) {
          return EXIT_FAILURE; // state module kdl should start with
                               // module_state {...}
        }
        break;
      case 1: // module level
        break;
      case 2: // dotfiles/(aur_)packages/(user) services/user hooks/ root hooks
              // level
        node_d2 = string_copy((char *)name_data);
        break;
      case 3: // child level
        if (memcmp(node_d2, "packages", 8) == 0) {
          char *package = string_copy((char *)name_data);
          darray_append(&module->packages, package);
        } else if (memcmp(node_d2, "aur_packages", 12) == 0) {
          char *aur_package = string_copy((char *)name_data);
          darray_append(&module->aur_packages, aur_package);
        } else if (memcmp(node_d2, "services", 8) == 0) {
          char *service =
              string_copy((char *)name_data); // implicit root=#false
          darray_append(&module->user_services, service);
          // for the hooks we go 1 depth deeper to get the property
        } else if (memcmp(node_d2, "pre_hooks", 9) == 0) {
          node_d3 = string_copy((char *)name_data);
        } else if (memcmp(node_d2, "post_hooks", 10) == 0) {
          node_d3 = string_copy((char *)name_data);
        }
        break;
      }
      depth += 1;
      break;
    case KDL_EVENT_END_NODE:
      depth -= 1;
      break;
    case KDL_EVENT_ARGUMENT:
      break;
    case KDL_EVENT_PROPERTY:
      // depth 3 for parsing the properties of dotfiles/hooks
      if (memcmp(node_d2, "dotfiles", 8) == 0) {
        if (value.boolean)
          module->to_link = true; // dotfiles link=#true
        else {
          module->to_link = false; // dotfiles link=#false
        }
      } else if (memcmp(node_d2, "pre_hooks", 9) == 0) {
        char *hook = string_copy(node_d3);
        if (value.boolean)
          darray_append(&module->pre_root_hooks, hook);
        else
          darray_append(&module->pre_user_hooks, hook);
      } else if (memcmp(node_d2, "post_hooks", 10) == 0) {
        char *hook = string_copy(node_d3);
        if (value.boolean)
          darray_append(&module->post_root_hooks, hook);
        else
          darray_append(&module->post_user_hooks, hook);
      }
      break;
    case KDL_EVENT_PARSE_ERROR:
      break;
    case KDL_EVENT_COMMENT:
      break;
    case KDL_EVENT_EOF:
      eof = true;
      break;
    }
    if (eof)
      break; // while break
  }

  if (node_d2 != nullptr) {
    free_sized(node_d2, strlen(node_d2));
  }

  kdl_destroy_parser(parser);

  return EXIT_SUCCESS;
}

int read_module(struct module *module, bool is_state) {
  char fidbuf[PATH_MAX];
  if (is_state) {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr/%s_state.kdl",
             get_user(), module->name);
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damgr/modules/%s.kdl",
             get_user(), module->name);
  }
  FILE *module_fid = fopen(fidbuf, "r");
  if (module_fid != nullptr) {
    LOG(LOG_INFO, "parsing module: %s", fidbuf);
    if (parse_module(module_fid, module) != EXIT_SUCCESS) {
      LOG(LOG_ERROR, "failed to parse module: %s", fidbuf);
      fclose(module_fid);
      return EXIT_FAILURE;
    }
    fclose(module_fid);
    return validate_module(*module, fidbuf);
  } else {
    if (is_state) {
      LOG(LOG_ERROR, "failed to open state module: %s", fidbuf);
      return EXIT_FAILURE;
    } else {
      LOG(LOG_ERROR, "failed to open new module: %s", fidbuf);
      return EXIT_FAILURE;
    }
  }
}

int write_module(struct module module) {
  char fidbuf[PATH_MAX];
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr/%s_state.kdl",
           get_user(), module.name);
  FILE *module_fid = fopen(fidbuf, "w");

  kdl_emitter_options e_opts = KDL_DEFAULT_EMITTER_OPTIONS;
  kdl_emitter *emitter =
      kdl_create_stream_emitter(&write_func, (void *)module_fid, &e_opts);

  kdl_emit_node(emitter, kdl_str_from_cstr("module_state"));
  kdl_start_emitting_children(emitter); // open module_state level
  kdl_emit_node(emitter, kdl_str_from_cstr(module.name));
  kdl_start_emitting_children(emitter); // open module level
  if (module.to_link) {
    kdl_emit_node(emitter, kdl_str_from_cstr("dotfiles"));
    kdl_value value = {.type = KDL_TYPE_BOOLEAN, .boolean = true};
    kdl_emit_property(emitter, kdl_str_from_cstr("link"), &value);
  }
  kdl_emit_node(emitter, kdl_str_from_cstr("packages"));
  kdl_start_emitting_children(emitter); // open packages level
  for (size_t i = 0; i < module.packages.count; ++i) {
    kdl_emit_node(emitter, kdl_str_from_cstr(module.packages.items[i]));
  }
  kdl_finish_emitting_children(emitter); // close packages level
  kdl_emit_node(emitter, kdl_str_from_cstr("aur_packages"));
  kdl_start_emitting_children(emitter); // open aur_packages level
  for (size_t i = 0; i < module.aur_packages.count; ++i) {
    kdl_emit_node(emitter, kdl_str_from_cstr(module.aur_packages.items[i]));
  }
  kdl_finish_emitting_children(emitter); // close aur_packages level
  kdl_emit_node(emitter, kdl_str_from_cstr("services"));
  kdl_start_emitting_children(emitter); // open services level
  for (size_t i = 0; i < module.user_services.count; ++i) {
    kdl_emit_node(emitter, kdl_str_from_cstr(module.user_services.items[i]));
  }
  kdl_finish_emitting_children(emitter); // close services level
  kdl_emit_node(emitter, kdl_str_from_cstr("pre_hooks"));
  kdl_start_emitting_children(emitter); // open pre hooks level
  for (size_t i = 0; i < module.pre_root_hooks.count; ++i) {
    kdl_emit_node(emitter, kdl_str_from_cstr(module.pre_root_hooks.items[i]));
    kdl_value value = {.type = KDL_TYPE_BOOLEAN, .boolean = true};
    kdl_emit_property(emitter, kdl_str_from_cstr("root"), &value);
  }
  for (size_t i = 0; i < module.pre_user_hooks.count; ++i) {
    kdl_emit_node(emitter, kdl_str_from_cstr(module.pre_user_hooks.items[i]));
    kdl_value value = {.type = KDL_TYPE_BOOLEAN, .boolean = false};
    kdl_emit_property(emitter, kdl_str_from_cstr("root"), &value);
  }
  kdl_finish_emitting_children(emitter); // close pre hooks level
  kdl_emit_node(emitter, kdl_str_from_cstr("post_hooks"));
  kdl_start_emitting_children(emitter); // open post hooks level
  for (size_t i = 0; i < module.post_root_hooks.count; ++i) {
    kdl_emit_node(emitter, kdl_str_from_cstr(module.post_root_hooks.items[i]));
    kdl_value value = {.type = KDL_TYPE_BOOLEAN, .boolean = true};
    kdl_emit_property(emitter, kdl_str_from_cstr("root"), &value);
  }
  for (size_t i = 0; i < module.post_user_hooks.count; ++i) {
    kdl_emit_node(emitter, kdl_str_from_cstr(module.post_user_hooks.items[i]));
    kdl_value value = {.type = KDL_TYPE_BOOLEAN, .boolean = false};
    kdl_emit_property(emitter, kdl_str_from_cstr("root"), &value);
  }
  kdl_finish_emitting_children(emitter); // close post hooks level
  kdl_finish_emitting_children(emitter); // close module level
  kdl_finish_emitting_children(emitter); // close module_state level
  kdl_emit_end(emitter);

  kdl_destroy_emitter(emitter);
  return EXIT_SUCCESS;
}
