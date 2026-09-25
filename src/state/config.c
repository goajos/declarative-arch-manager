#include "../../include/kdl/kdl.h"
#include "../state.h"
#include "../utils.h"
#include <stdlib.h>

int get_config([[maybe_unused]] struct config *config, bool state) {
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
    parse_config(config_fid, config);
    fclose(config_fid);
  } else {
    if (state) {
      // perror("failed to open state config!");
      fprintf(stderr, "failed to open state config: %s\n", fidbuf);
      return EXIT_FAILURE;
    } else {
      // perror("failed to open new config!");
      fprintf(stderr, "failed to open new config: %s\n", fidbuf);
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int parse_config(FILE *fid, [[maybe_unused]] struct config *config) {
  [[maybe_unused]] kdl_parser *parser =
      kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS);

  puts("succesfully created a parser...");

  return EXIT_SUCCESS;
}
