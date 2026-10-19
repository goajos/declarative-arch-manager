#include "damgr/paths.h"

void damgr_paths_init(Damgr_Paths *paths, const char *home) {
  snprintf(paths->home, sizeof(paths->home), "%s", home);
  snprintf(paths->active_dir, sizeof(paths->active_dir), "%s/.config/damgr",
           home);
  snprintf(paths->active_config, sizeof(paths->active_config),
           "%s/.config/damgr/config.conf", home);
  snprintf(paths->active_host_dir, sizeof(paths->active_host_dir),
           "%s/.config/damgr/hosts", home);
  snprintf(paths->active_module_dir, sizeof(paths->active_module_dir),
           "%s/.config/damgr/modules", home);
  snprintf(paths->active_dotfile_dir, sizeof(paths->active_dotfile_dir),
           "%s/.config/damgr/dotfiles", home);
  snprintf(paths->state_dir, sizeof(paths->state_dir), "%s/.local/state/damgr",
           home);
  snprintf(paths->staging_dir, sizeof(paths->staging_dir),
           "%s/.local/state/damgr/.staging", home);
  snprintf(paths->backup_dir, sizeof(paths->backup_dir),
           "%s/.local/state/damgr/.backup", home);
  snprintf(paths->journal, sizeof(paths->journal),
           "%s/.local/state/damgr/.journal", home);
}

static void append_conf(const char *prefix, const char *name, char *out,
                        size_t out_size) {
  snprintf(out, out_size, "%s/%s.conf", prefix, name);
}

static void append_conf_prefixed(const char *prefix, const char *name,
                                 const char *file_prefix, char *out,
                                 size_t out_size) {
  snprintf(out, out_size, "%s/%s%s.conf", prefix, file_prefix, name);
}

static void append(const char *prefix, const char *name, char *out,
                   size_t out_size) {
  snprintf(out, out_size, "%s/%s", prefix, name);
}

void damgr_paths_active_host_conf(const Damgr_Paths *paths, const char *name,
                                  char *out, size_t out_size) {
  append_conf(paths->active_host_dir, name, out, out_size);
}

void damgr_paths_active_module_conf(const Damgr_Paths *paths, const char *name,
                                    char *out, size_t out_size) {
  append_conf(paths->active_module_dir, name, out, out_size);
}

void damgr_paths_state_conf(const Damgr_Paths *paths, const char *name,
                            char *out, size_t out_size) {
  append_conf_prefixed(paths->state_dir, name, "state_", out, out_size);
}

void damgr_paths_staging_conf(const Damgr_Paths *paths, const char *name,
                              char *out, size_t out_size) {
  append_conf(paths->staging_dir, name, out, out_size);
}

void damgr_paths_dotfile_src(const Damgr_Paths *paths, const char *name,
                             char *out, size_t out_size) {
  append(paths->active_dotfile_dir, name, out, out_size);
}

void damgr_paths_dotfile_dst(const Damgr_Paths *paths, const char *name,
                             char *out, size_t out_size) {
  // special case: dst is ~/.config/<name>, not a damgr-managed dir
  char config_dir[DAMGR_FID_BUF_MAX];
  snprintf(config_dir, sizeof(config_dir), "%s/.config", paths->home);
  append(config_dir, name, out, out_size);
}
