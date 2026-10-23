#ifndef DAMGR_PATHS_H
#define DAMGR_PATHS_H
#include <stdio.h>

#include "damgr/result.h"

constexpr size_t PATH_BUFFER_MAX = 4096;

typedef struct paths {
  char home[PATH_BUFFER_MAX];
  char active_dir[PATH_BUFFER_MAX];         // ~/.config/damgr
  char active_config[PATH_BUFFER_MAX];      // ~/.config/damgr/config.conf
  char active_host_dir[PATH_BUFFER_MAX];    // ~/.config/damgr/hosts
  char active_module_dir[PATH_BUFFER_MAX];  // ~/.config/damgr/modules
  char active_dotfile_dir[PATH_BUFFER_MAX]; // ~/.config/damgr/dotfiles
  char state_dir[PATH_BUFFER_MAX];          // ~/.local/state/damgr
  char state_config[PATH_BUFFER_MAX]; // ~/.local/state/damgr/state_config.conf
  char tmp_dir[PATH_BUFFER_MAX];      // ~/.local/state/damgr/.tmp
  char journal[PATH_BUFFER_MAX];      // ~/.local/state/damgr/.journal
} Damgr_Paths;

Result_t damgr_paths_init_r(Damgr_Paths *paths, const char *home);

Result_t damgr_resolve_host_path_r(const Damgr_Paths *paths, char *fid_buffer,
                                   size_t size, const char *host_name,
                                   bool is_state);

Result_t damgr_resolve_module_path_r(const Damgr_Paths *paths, char *fid_buffer,
                                     size_t size, const char *module_name,
                                     bool is_state);

#ifdef DAMGR_PATHS_IMPLEMENTATION

Result_t damgr_paths_init_r(Damgr_Paths *paths, const char *home) {
  DAMGR_CALL(damgr_snprintf_r(paths->home, sizeof(paths->home), "%s", home));
  DAMGR_CALL(damgr_snprintf_r(paths->active_dir, sizeof(paths->active_dir),
                              "%s/.config/damgr", home));
  DAMGR_CALL(damgr_snprintf_r(paths->active_config,
                              sizeof(paths->active_config),
                              "%s/.config/damgr/config.conf", home));
  DAMGR_CALL(damgr_snprintf_r(paths->active_host_dir,
                              sizeof(paths->active_host_dir),
                              "%s/.config/damgr/hosts", home));
  DAMGR_CALL(damgr_snprintf_r(paths->active_module_dir,
                              sizeof(paths->active_module_dir),
                              "%s/.config/damgr/modules", home));
  DAMGR_CALL(damgr_snprintf_r(paths->active_dotfile_dir,
                              sizeof(paths->active_dotfile_dir),
                              "%s/.config/damgr/dotfiles", home));
  DAMGR_CALL(damgr_snprintf_r(paths->state_dir, sizeof(paths->state_dir),
                              "%s/.local/state/damgr", home));
  DAMGR_CALL(damgr_snprintf_r(paths->state_config, sizeof(paths->state_config),
                              "%s/.local/state/damgr/state_config.conf", home));
  DAMGR_CALL(damgr_snprintf_r(paths->tmp_dir, sizeof(paths->tmp_dir),
                              "%s/.local/state/damgr/.tmp", home));
  DAMGR_CALL(damgr_snprintf_r(paths->journal, sizeof(paths->journal),
                              "%s/.local/state/damgr/.journal", home));
  return Success;
}

static Result_t append_conf_r(const char *path, const char *name,
                              char *fid_buffer, size_t size, bool is_state) {
  if (is_state) {
    DAMGR_CALL(
        damgr_snprintf_r(fid_buffer, size, "%s/state_%s.conf", path, name));
  } else {
    DAMGR_CALL(damgr_snprintf_r(fid_buffer, size, "%s/%s.conf", path, name));
  }
  return Success;
}

Result_t damgr_resolve_host_path_r(const Damgr_Paths *paths, char *fid_buffer,
                                   size_t size, const char *host_name,
                                   bool is_state) {
  if (is_state) {
    DAMGR_CALL(
        append_conf_r(paths->state_dir, host_name, fid_buffer, size, is_state));
  } else {
    DAMGR_CALL(append_conf_r(paths->active_host_dir, host_name, fid_buffer,
                             size, is_state));
  }
  return Success;
}

Result_t damgr_resolve_module_path_r(const Damgr_Paths *paths, char *fid_buffer,
                                     size_t size, const char *module_name,
                                     bool is_state) {
  if (is_state) {
    DAMGR_CALL(append_conf_r(paths->state_dir, module_name, fid_buffer, size,
                             is_state));
  } else {
    DAMGR_CALL(append_conf_r(paths->active_module_dir, module_name, fid_buffer,
                             size, is_state));
  }
  return Success;
}
#endif /* DAMGR_PATHS_IMPLEMENTATION */
#endif /* DAMGR_PATHS_H */
