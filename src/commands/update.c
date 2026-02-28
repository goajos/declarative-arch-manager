#include "damgr/log.h"
#include "damgr/utils.h"
#include <stdlib.h>

int damgr_update() {
  LOG(LOG_INFO, "running damgr update...");

  struct config new_config = {};
  if (read_config(&new_config, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  };
  if (new_config.aur_helper) {
    LOG(LOG_INFO, "executing aur update...");
    execute_aur_update_command(new_config.aur_helper);
  } else {
    LOG(LOG_INFO, "executing pacman update...");
    execute_update_command();
  }

  return EXIT_SUCCESS;
}
