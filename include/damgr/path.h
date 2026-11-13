#ifndef DAMGR_PATH_H
#define DAMGR_PATH_H
#include <stdio.h>

#include "damgr/utils.h"

typedef struct paths {
  char home[DAMGR_PATH_BUFFER_MAX];
  char active_dir[DAMGR_PATH_BUFFER_MAX];         // ~/.config/damgr
  char active_config[DAMGR_PATH_BUFFER_MAX];      // ~/.config/damgr/config.conf
  char active_host_dir[DAMGR_PATH_BUFFER_MAX];    // ~/.config/damgr/hosts
  char active_module_dir[DAMGR_PATH_BUFFER_MAX];  // ~/.config/damgr/modules
  char active_dotfile_dir[DAMGR_PATH_BUFFER_MAX]; // ~/.config/damgr/dotfiles
  char state_dir[DAMGR_PATH_BUFFER_MAX];          // ~/.local/state/damgr
  char state_config
      [DAMGR_PATH_BUFFER_MAX];         // ~/.local/state/damgr/state_config.conf
  char tmp_dir[DAMGR_PATH_BUFFER_MAX]; // ~/.local/state/damgr/.tmp
  char journal[DAMGR_PATH_BUFFER_MAX]; // ~/.local/state/damgr/.journal
} Damgr_Paths;

Result_t damgr_init_paths_result(Damgr_Paths *paths, const char *home);
Result_t damgr_resolve_host_path_result(const Damgr_Paths *paths,
                                        char *fid_buffer, size_t size,
                                        const char *host_name, bool is_state);

Result_t damgr_resolve_module_path_result(const Damgr_Paths *paths,
                                          char *fid_buffer, size_t size,
                                          const char *module_name,
                                          bool is_state);

#ifdef DAMGR_PATH_IMPLEMENTATION

Result_t damgr_init_paths_result(Damgr_Paths *paths, const char *home) {
  DAMGR_CALL(
      damgr_snprintf_result(paths->home, sizeof(paths->home), "%s", home));
  DAMGR_CALL(damgr_snprintf_result(paths->active_dir, sizeof(paths->active_dir),
                                   "%s/.config/damgr", home));
  DAMGR_CALL(damgr_snprintf_result(paths->active_config,
                                   sizeof(paths->active_config),
                                   "%s/.config/damgr/config.conf", home));
  DAMGR_CALL(damgr_snprintf_result(paths->active_host_dir,
                                   sizeof(paths->active_host_dir),
                                   "%s/.config/damgr/hosts", home));
  DAMGR_CALL(damgr_snprintf_result(paths->active_module_dir,
                                   sizeof(paths->active_module_dir),
                                   "%s/.config/damgr/modules", home));
  DAMGR_CALL(damgr_snprintf_result(paths->active_dotfile_dir,
                                   sizeof(paths->active_dotfile_dir),
                                   "%s/.config/damgr/dotfiles", home));
  DAMGR_CALL(damgr_snprintf_result(paths->state_dir, sizeof(paths->state_dir),
                                   "%s/.local/state/damgr", home));
  DAMGR_CALL(
      damgr_snprintf_result(paths->state_config, sizeof(paths->state_config),
                            "%s/.local/state/damgr/state_config.conf", home));
  DAMGR_CALL(damgr_snprintf_result(paths->tmp_dir, sizeof(paths->tmp_dir),
                                   "%s/.local/state/damgr/.tmp", home));
  DAMGR_CALL(damgr_snprintf_result(paths->journal, sizeof(paths->journal),
                                   "%s/.local/state/damgr/.journal", home));
  return Success;
}

static Result_t append_conf_result(const char *path, const char *name,
                                   char *fid_buffer, size_t size,
                                   bool is_state) {
  if (is_state) {
    DAMGR_CALL(damgr_snprintf_result(fid_buffer, size, "%s/state_%s.conf", path,
                                     name));
  } else {
    DAMGR_CALL(
        damgr_snprintf_result(fid_buffer, size, "%s/%s.conf", path, name));
  }
  return Success;
}

Result_t damgr_resolve_host_path_result(const Damgr_Paths *paths,
                                        char *fid_buffer, size_t size,
                                        const char *host_name, bool is_state) {
  if (is_state) {
    DAMGR_CALL(append_conf_result(paths->state_dir, host_name, fid_buffer, size,
                                  is_state));
  } else {
    DAMGR_CALL(append_conf_result(paths->active_host_dir, host_name, fid_buffer,
                                  size, is_state));
  }
  return Success;
}

Result_t damgr_resolve_module_path_result(const Damgr_Paths *paths,
                                          char *fid_buffer, size_t size,
                                          const char *module_name,
                                          bool is_state) {
  if (is_state) {
    DAMGR_CALL(append_conf_result(paths->state_dir, module_name, fid_buffer,
                                  size, is_state));
  } else {
    DAMGR_CALL(append_conf_result(paths->active_module_dir, module_name,
                                  fid_buffer, size, is_state));
  }
  return Success;
}

#endif /* DAMGR_PATH_IMPLEMENTATION */
#endif /* DAMGR_PATH_H */
