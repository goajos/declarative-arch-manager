#include "damgr/log.h"
#include "damgr/state.h"
#include "damgr/utils.h"
#include "kdl/kdl.h"
#include <stdlib.h>
#include <string.h>

static int validate_host(struct host host, char *fidbuf) {
  // TODO: is there any more validation to do for the host?
  if (host.modules.count == 0) {
    damgr_log(ERROR, "no modules found for active host: %s", fidbuf);
    return EXIT_FAILURE;
  }
  damgr_log(INFO, "successfully parsed host: %s", fidbuf);
  return EXIT_SUCCESS;
}

int parse_host(FILE *fid, struct host *host) {
  kdl_parser *parser =
      kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS);

  size_t depth = 0;
  char *node_d2 = nullptr;
  bool eof = false;

  int ret;
  while (true) {
    kdl_event_data *next_event = kdl_parser_next_event(parser);
    kdl_event event = next_event->event;
    const char *name_data = next_event->name.data;
    switch (event) {
    case KDL_EVENT_START_NODE:
      switch (depth) {
      case 0: // host(_state) level
        // reading host
        if (strlen(name_data) == 4 && memcmp(name_data, "host", 4) != 0) {
          ret = EXIT_FAILURE; // host kdl should start with host {...}
          goto cleanup;
        }
        // reading state host
        if (strlen(name_data) == 10 &&
            memcmp(name_data, "host_state", 10) != 0) {
          ret = EXIT_FAILURE; // state host kdl should start with host_state
                              // {...}
          goto cleanup;
        }
        break;
      case 1: // host level
        break;
      case 2: // modules/(root) services level
        node_d2 = string_copy((char *)name_data);
        break;
      case 3: // child level
        if (memcmp(node_d2, "modules", 7) == 0) {
          struct module module = {.name = string_copy((char *)name_data)};
          modules_append(&host->modules, module);
        } else if (memcmp(node_d2, "services", 8) == 0) {
          char *service = string_copy((char *)name_data); // implicit root=#true
          darray_append(&host->root_services, service);
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

  ret = EXIT_SUCCESS;

cleanup:
  if (node_d2 != nullptr) {
    free(node_d2);
  }
  kdl_destroy_parser(parser);

  return ret;
}

int read_host(char *user, struct host *host, bool is_state) {
  char fidbuf[PATH_MAX];
  if (is_state) {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr/%s_state.kdl",
             user, host->name);
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damgr/hosts/%s.kdl",
             user, host->name);
  }
  FILE *host_fid = fopen(fidbuf, "r");
  if (host_fid != nullptr) {
    damgr_log(INFO, "parsing host: %s", fidbuf);
    if (parse_host(host_fid, host) != EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to parse host: %s", fidbuf);
      fclose(host_fid);
      return EXIT_FAILURE;
    }
    fclose(host_fid);
    return validate_host(*host, fidbuf);
  } else {
    char *fmt = (is_state) ? "state" : "new";
    damgr_log(ERROR, "failed to open %s host: %s", fmt, fidbuf);
    return EXIT_FAILURE;
  }
}

int write_host(char *user, struct host host) {
  char fidbuf[PATH_MAX];
  snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr/%s_state.kdl",
           user, host.name);
  FILE *host_fid = fopen(fidbuf, "w");

  kdl_emitter_options e_opts = KDL_DEFAULT_EMITTER_OPTIONS;
  kdl_emitter *emitter =
      kdl_create_stream_emitter(&write_func, (void *)host_fid, &e_opts);

  kdl_emit_node(emitter, kdl_str_from_cstr("host_state"));
  kdl_start_emitting_children(emitter); // open host_state level
  kdl_emit_node(emitter, kdl_str_from_cstr(host.name));
  kdl_start_emitting_children(emitter); // open host level
  kdl_emit_node(emitter, kdl_str_from_cstr("modules"));
  kdl_start_emitting_children(emitter); // open modules level
  for (size_t i = 0; i < host.modules.count; ++i) {
    if (host.modules.items[i].boolean.is_done) {
      kdl_emit_node(emitter, kdl_str_from_cstr(host.modules.items[i].name));
    }
  }
  kdl_finish_emitting_children(emitter); // close modules level
  kdl_emit_node(emitter, kdl_str_from_cstr("services"));
  kdl_start_emitting_children(emitter); // open services level
  for (size_t i = 0; i < host.root_services.count; ++i) {
    kdl_emit_node(emitter, kdl_str_from_cstr(host.root_services.items[i]));
  }
  kdl_finish_emitting_children(emitter); // close services level
  kdl_finish_emitting_children(emitter); // close host level
  kdl_finish_emitting_children(emitter); // clsoe host_state level
  kdl_emit_end(emitter);

  kdl_destroy_emitter(emitter);
  fclose(host_fid);
  return EXIT_SUCCESS;
}
