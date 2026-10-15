#include "damgr/log.h"
#include "damgr/state.h"
#include <errno.h>
#include <sys/stat.h>

static int stage_config(const Damgr_Config *config, const char *fidbuf) {
  FILE *config_fid = fopen(fidbuf, "w");
  if (config_fid == nullptr) {
    damgr_log(ERROR, "could not open staging config fid %s: %s", fidbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  fprintf(config_fid, "active_host=%s\n", config->active_host->host_name);

  fclose(config_fid);
  return EXIT_SUCCESS;
}

static int stage_host(const Damgr_Host *host, const char *fidbuf) {
  FILE *host_fid = fopen(fidbuf, "w");
  if (host_fid == nullptr) {
    damgr_log(ERROR, "could not open staging host fid %s: %s", fidbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  fprintf(host_fid, "modules=\n");
  for (size_t i = 0; i < host->modules_cnt; ++i)
    fprintf(host_fid, "\t%s\n", host->modules[i]->module_name);

  fclose(host_fid);
  return EXIT_SUCCESS;
}

static int stage_module(const Damgr_Module *module, const char *fidbuf) {
  FILE *module_fid = fopen(fidbuf, "w");
  if (module_fid == nullptr) {
    damgr_log(ERROR, "could not open staging module fid %s: %s", fidbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  fprintf(module_fid, "dotfiles=%s\n", module->to_link ? "true" : "false");
  if (module->packages_cnt > 0) {
    fprintf(module_fid, "packages=\n");
    for (size_t i = 0; i < module->packages_cnt; ++i)
      fprintf(module_fid, "\t%s\n", module->packages[i]);
  }
  if (module->aur_packages_cnt > 0) {
    fprintf(module_fid, "aur_packages=\n");
    for (size_t i = 0; i < module->aur_packages_cnt; ++i)
      fprintf(module_fid, "\t%s\n", module->aur_packages[i]);
  }
  if (module->root_services_cnt > 0 || module->user_services_cnt > 0) {
    fprintf(module_fid, "services=\n");
    for (size_t i = 0; i < module->root_services_cnt; ++i)
      fprintf(module_fid, "\t%s:true\n", module->root_services[i]);
    for (size_t i = 0; i < module->user_services_cnt; ++i)
      fprintf(module_fid, "\t%s:false\n", module->user_services[i]);
  }

  fclose(module_fid);
  return EXIT_SUCCESS;
}

// TODO: clear .staging before mkdir?
// if (errno == EEXIST) {
//   remove(stagingbuf);
// }
// staging results in a .staging folder with all active state present
int damgr_stage(const Damgr_Config *config) {
  // TODO: clean the .staging folder before writing again?
  char stagingbuf[DAMGR_PATH_MAX];
  snprintf(stagingbuf, sizeof(stagingbuf), "%s/.local/state/damgr/.staging",
           getenv("HOME"));
  if (mkdir(stagingbuf, 0755) == -1) {
    damgr_log(ERROR, "could not create .staging dir: %s", strerror(errno));
    return EXIT_FAILURE;
  }

  int ret;
  char fidbuf[DAMGR_PATH_MAX];
  snprintf(fidbuf, sizeof(fidbuf),
           "%s/.local/state/damgr/.staging/config_state.conf", getenv("HOME"));
  ret = stage_config(config, fidbuf);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  snprintf(fidbuf, sizeof(fidbuf),
           "%s/.local/state/damgr/.staging/%s_state.conf", getenv("HOME"),
           config->active_host->host_name);
  ret = stage_host(config->active_host, fidbuf);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  for (size_t i = 0; i < config->active_host->modules_cnt; ++i) {
    snprintf(fidbuf, sizeof(fidbuf),
             "%s/.local/state/damgr/.staging/%s_state.conf", getenv("HOME"),
             config->active_host->modules[i]->module_name);
    ret = stage_module(config->active_host->modules[i], fidbuf);
    if (ret == EXIT_FAILURE)
      return EXIT_FAILURE;
  }

  damgr_log(INFO, "staging finished: %zu module(s) written to %s",
            config->active_host->modules_cnt, stagingbuf);
  return ret;
}
