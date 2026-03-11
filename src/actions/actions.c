#include "damgr/log.h"
#include "damgr/state.h"
#include <string.h>

int damgr_get_actions(Damgr_Config *old_config, Damgr_Config *config) {
  if (old_config->active_host.name != nullptr) {
    int ret = strcmp(old_config->active_host.name, config->active_host.name);
    if (ret < 0 || ret > 0) { // different host
      if (damgr_get_actions_from_host(&config->active_host) != EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to get actions for the new host: %s",
                  config->active_host.name);
        return EXIT_FAILURE;
      }
    } else { // same host
      if (damgr_get_actions_from_hosts_diff(
              &old_config->active_host, &config->active_host) != EXIT_SUCCESS) {
        damgr_log(ERROR, "failed to get actions comparing the hosts: %s",
                  config->active_host.name);
        return EXIT_FAILURE;
      }
    }
  } else { // no state host
    if (damgr_get_actions_from_host(&config->active_host) != EXIT_SUCCESS) {
      damgr_log(ERROR, "failed to get actions for the new host: %s",
                config->active_host.name);
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
