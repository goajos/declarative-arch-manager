#include "damgr/log.h"
#include "damgr/utils.h"
#include <stdlib.h>

int damgr_update() {
  damgr_log(INFO, "running damgr update...");

  char *user = get_user();
  if (user == nullptr) {
    return EXIT_FAILURE;
  }
  struct config new_config = {};
  if (read_config(user, &new_config, false) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  };
  if (new_config.aur_helper) {
    damgr_log(INFO, "executing aur update...");
    if (execute_aur_update_command(new_config.aur_helper) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  } else {
    damgr_log(INFO, "executing pacman update...");
    if (execute_update_command() != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
