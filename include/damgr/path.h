#ifndef DAMGR_PATH_H
#define DAMGR_PATH_H
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

Result_t damgr_init_paths_result(Damgr_Paths *paths, const char *home);

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

#endif /* DAMGR_PATH_IMPLEMENTATION */
#endif /* DAMGR_PATH_H */
