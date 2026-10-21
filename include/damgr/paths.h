#ifndef DAMGR_PATHS_H
#define DAMGR_PATHS_H
#include <stdio.h>

#ifndef DAMGR_RESULT_H
#include "damgr/result.h"
#endif

constexpr size_t DAMGR_PATH_BUFFER_MAX = 4096;

typedef struct paths {
  char home[DAMGR_PATH_BUFFER_MAX];
  char active_dir[DAMGR_PATH_BUFFER_MAX];         // ~/.config/damgr
  char active_config[DAMGR_PATH_BUFFER_MAX];      // ~/.config/damgr/config.conf
  char active_host_dir[DAMGR_PATH_BUFFER_MAX];    // ~/.config/damgr/hosts
  char active_module_dir[DAMGR_PATH_BUFFER_MAX];  // ~/.config/damgr/modules
  char active_dotfile_dir[DAMGR_PATH_BUFFER_MAX]; // ~/.config/damgr/dotfiles
  char state_dir[DAMGR_PATH_BUFFER_MAX];          // ~/.local/state/damgr
  char staging_dir[DAMGR_PATH_BUFFER_MAX]; // ~/.local/state/damgr/.staging
  char backup_dir[DAMGR_PATH_BUFFER_MAX];  // ~/.local/state/damgr/.backup
  char journal[DAMGR_PATH_BUFFER_MAX];     // ~/.local/state/damgr/.journal
} Damgr_Paths;

Result_t(Nullptr_t, ErrMessage_t)
    damgr_paths_init(Damgr_Paths *paths, const char *home);

#ifdef DAMGR_PATHS_IMPLEMENTATION

Result_t(Nullptr_t, ErrMessage_t)
    damgr_paths_init(Damgr_Paths *paths, const char *home) {
  Result_t(Nullptr_t, ErrMessage_t) result;
  result = snprintf_r(paths->home, sizeof(paths->home), "%s", home);
  result = snprintf_r(paths->active_dir, sizeof(paths->active_dir),
                      "%s/.config/damgr", home);
  result = snprintf_r(paths->active_config, sizeof(paths->active_config),
                      "%s/.config/damgr/config.conf", home);
  result = snprintf_r(paths->active_host_dir, sizeof(paths->active_host_dir),
                      "%s/.config/damgr/hosts", home);
  result =
      snprintf_r(paths->active_module_dir, sizeof(paths->active_module_dir),
                 "%s/.config/damgr/modules", home);
  result =
      snprintf_r(paths->active_dotfile_dir, sizeof(paths->active_dotfile_dir),
                 "%s/.config/damgr/dotfiles", home);
  result = snprintf_r(paths->state_dir, sizeof(paths->state_dir),
                      "%s/.local/state/damgr", home);
  result = snprintf_r(paths->staging_dir, sizeof(paths->staging_dir),
                      "%s/.local/state/damgr/.staging", home);
  result = snprintf_r(paths->backup_dir, sizeof(paths->backup_dir),
                      "%s/.local/state/damgr/.backup", home);
  result = snprintf_r(paths->journal, sizeof(paths->journal),
                      "%s/.local/state/damgr/.journal", home);
  return result;
}

#endif /* DAMGR_PATHS_IMPLEMENTATION */
#endif /* DAMGR_PATHS_H */
