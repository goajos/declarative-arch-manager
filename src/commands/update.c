#include "damgr/log.h"
#include "damgr/utils.h"
#include <stdlib.h>

int damgr_update() {
  damgr_log(INFO, "running damgr update...");

  char *user = damgr_get_user();
  if (user == nullptr) {
    return EXIT_FAILURE;
  }
  int ret = EXIT_FAILURE;
  Damgr_Config new_config = {};
  if (damgr_read_config(user, &new_config, false) != EXIT_SUCCESS) {
    goto cleanup;
  };
  if (new_config.aur_helper) {
    damgr_log(INFO, "executing aur update...");
    if (damgr_execute_aur_update_command(new_config.aur_helper) !=
        EXIT_SUCCESS) {
      goto cleanup;
    }
  } else {
    damgr_log(INFO, "executing pacman update...");
    if (damgr_execute_update_command() != EXIT_SUCCESS) {
      goto cleanup;
    }
  }

  ret = EXIT_SUCCESS;

cleanup:
  damgr_free_config(&new_config);
  return ret;
}
