#include "damgr/stage.h"
#include "damgr/utils.h"

static int stage_context(const Damgr_Context *context, const char *fidbuf) {
  FILE *context_fid = fopen(fidbuf, "w");
  if (context_fid == nullptr) {
    damgr_log(ERROR, "could not open staging config %s: %s", fidbuf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  fprintf(context_fid, "active_host=%s\n", context->active_host->host_name);

  fclose(context_fid);
  return EXIT_SUCCESS;
}

static int stage_host(const Damgr_Host *host, const char *fidbuf) {
  FILE *host_fid = fopen(fidbuf, "w");
  if (host_fid == nullptr) {
    damgr_log(ERROR, "could not open staging host %s: %s", fidbuf,
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
    damgr_log(ERROR, "could not open staging module %s: %s", fidbuf,
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

int damgr_stage_with_context(const Damgr_Context *context) {
  int ret;
  char fid_buf[DAMGR_FID_BUF_MAX];
  snprintf(fid_buf, sizeof(fid_buf), "%s/.local/state/damgr/.staging",
           getenv("HOME"));
  // TODO: do we also want to clear the dir here?
  ret = damgr_make_dir(fid_buf);
  if (ret == EXIT_FAILURE)
    return ret;

  damgr_format_fid_buf(fid_buf, "config", true, NO_ACTIVE_DIR, STAGING);
  ret = stage_context(context, fid_buf);
  if (ret == EXIT_FAILURE)
    return ret;

  damgr_format_fid_buf(fid_buf, context->active_host->host_name, true,
                       NO_ACTIVE_DIR, STAGING);
  ret = stage_host(context->active_host, fid_buf);
  if (ret == EXIT_FAILURE)
    return ret;

  for (size_t i = 0; i < context->active_host->modules_cnt; ++i) {
    damgr_format_fid_buf(fid_buf, context->active_host->modules[i]->module_name,
                         true, NO_ACTIVE_DIR, STAGING);
    ret = stage_module(context->active_host->modules[i], fid_buf);
    if (ret == EXIT_FAILURE)
      return ret;
  }

  return EXIT_SUCCESS;
}

int damgr_stage(Damgr_Arena *arena) {
  int ret;

  Damgr_Context context = {0};
  ret = damgr_read_active_context(&context, arena);
  if (ret == EXIT_FAILURE)
    return ret;

  ret = damgr_stage_with_context(&context);
  if (ret == EXIT_FAILURE)
    return ret;

  return EXIT_SUCCESS;
}
