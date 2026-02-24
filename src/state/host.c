#include "../../include/kdl/kdl.h"
#include "../logging.h"
#include "../state.h"
#include "../utils.h"
#include <stdlib.h>
#include <string.h>

static int validate_host(struct host host, char *fidbuf) {
  // TODO: root services can be empty?
  if (host.root_services.count == 0) {
    LOG(LOG_ERROR, "failed to parse root_services for host: %s", fidbuf);
    return EXIT_FAILURE;
  }
  if (host.modules.count == 0) {
    LOG(LOG_ERROR, "failed to parse modules for host: %s", fidbuf);
    return EXIT_FAILURE;
  }
  LOG(LOG_INFO, "successfully parsed host: %s", fidbuf);
  return EXIT_SUCCESS;
}

int get_host(struct host *host, bool state) {
  char fidbuf[PATH_MAX];
  if (state) {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr/%s_state.kdl",
             get_user(), host->name);
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damgr/hosts/%s.kdl",
             get_user(), host->name);
  }
  FILE *host_fid = fopen(fidbuf, "r");
  if (host_fid != nullptr) {
    LOG(LOG_INFO, "parsing host: %s", fidbuf);
    if (parse_host(host_fid, host) != EXIT_SUCCESS) {
      LOG(LOG_ERROR, "failed to parse host: %s", fidbuf);
      fclose(host_fid);
      return EXIT_FAILURE;
    }
    fclose(host_fid);
    return validate_host(*host, fidbuf);
  } else {
    if (state) {
      LOG(LOG_ERROR, "failed to open state host: %s", fidbuf);
      return EXIT_FAILURE;
    } else {
      LOG(LOG_ERROR, "failed to open new host: %s", fidbuf);
      return EXIT_FAILURE;
    }
  }
}

int parse_host(FILE *fid, struct host *host) {
  kdl_parser *parser =
      kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS);

  size_t depth = 0;
  char *node_d2 = nullptr;
  bool eof = false;

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
          return EXIT_FAILURE; // host kdl should start with host {...}
        }
        // reading state host
        if (strlen(name_data) == 10 &&
            memcmp(name_data, "host_state", 10) != 0) {
          return EXIT_FAILURE; // state host kdl should start with host_state
                               // {...}
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

  if (node_d2 != nullptr) {
    free_sized(node_d2, strlen(node_d2));
  }
  kdl_destroy_parser(parser);

  return EXIT_SUCCESS;
}
