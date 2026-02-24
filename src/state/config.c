#include "../../include/kdl/kdl.h"
#include "../logging.h"
#include "../state.h"
#include "../utils.h"
#include <stdlib.h>
#include <string.h>

static int validate_config(struct config config, char *fidbuf) {
  // TODO: aur helper can be empty?
  if (config.aur_helper == nullptr) {
    LOG(LOG_ERROR, "failed to parse aur_helper for config: %s", fidbuf);
    return EXIT_FAILURE;
  }
  if (config.active_host.name == nullptr) {
    LOG(LOG_ERROR, "failed to parse host name for config: %s", fidbuf);
    return EXIT_FAILURE;
  }
  LOG(LOG_INFO, "successfully parsed config: %s", fidbuf);
  return EXIT_SUCCESS;
}

int get_config(struct config *config, bool state) {
  char fidbuf[PATH_MAX];
  if (state) {
    snprintf(fidbuf, sizeof(fidbuf),
             "/home/%s/.local/state/damgr/config_state.kdl", get_user());
  } else {
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damgr/config.kdl",
             get_user());
  }
  FILE *config_fid = fopen(fidbuf, "r");
  if (config_fid != nullptr) {
    LOG(LOG_INFO, "parsing config: %s", fidbuf);
    if (parse_config(config_fid, config) != EXIT_SUCCESS) {
      LOG(LOG_ERROR, "failed to parse config: %s", fidbuf);
      fclose(config_fid);
      return EXIT_FAILURE;
    }
    fclose(config_fid);
    return validate_config(*config, fidbuf);
  } else {
    if (state) {
      LOG(LOG_ERROR, "failed to open state config: %s", fidbuf);
      return EXIT_FAILURE;
    } else {
      LOG(LOG_ERROR, "failed to open new config: %s", fidbuf);
      return EXIT_FAILURE;
    }
  }
}

int parse_config(FILE *fid, struct config *config) {
  kdl_parser *parser =
      kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS);

  size_t depth = 0;
  char *node_d1 = nullptr;
  bool eof = false;

  while (true) {
    kdl_event_data *next_event = kdl_parser_next_event(parser);
    kdl_event event = next_event->event;
    const char *name_data = next_event->name.data;
    kdl_value value = next_event->value;
    switch (event) {
    case KDL_EVENT_START_NODE:
      switch (depth) {
      case 0: // config(_state) level
        // reading config
        if (strlen(name_data) == 6 && memcmp(name_data, "config", 6) != 0) {
          return EXIT_FAILURE; // config kdl should start with config {...}
        }
        // reading state config
        if (strlen(name_data) == 12 &&
            memcmp(name_data, "config_state", 12) != 0) {
          return EXIT_FAILURE; // state config kdl should start with
                               // config_state {...}
        }
        break;
      case 1: // aur_helper/active_host level
        node_d1 = string_copy((char *)name_data);
        break;
      }
      depth += 1;
      break;
    case KDL_EVENT_END_NODE:
      depth -= 1;
      break;
    case KDL_EVENT_ARGUMENT:
      if (memcmp(node_d1, "aur_helper", 10) == 0) {
        config->aur_helper = string_copy((char *)value.string.data);
      } else if (memcmp(node_d1, "active_host", 11) == 0) {
        struct host host = {.name = string_copy((char *)value.string.data)};
        config->active_host = host;
      }
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
  if (node_d1 != nullptr) {
    free_sized(node_d1, strlen(node_d1));
  }
  kdl_destroy_parser(parser);

  return EXIT_SUCCESS;
}
