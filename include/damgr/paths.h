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
  char state_config
      [DAMGR_PATH_BUFFER_MAX]; // ~/.local/state/damgr/state_config.conf
  char staging_dir[DAMGR_PATH_BUFFER_MAX]; // ~/.local/state/damgr/.staging
  char backup_dir[DAMGR_PATH_BUFFER_MAX];  // ~/.local/state/damgr/.backup
  char journal[DAMGR_PATH_BUFFER_MAX];     // ~/.local/state/damgr/.journal
} Damgr_Paths;

Result_t(Nullptr_t, ErrMessage_t)
    damgr_paths_init_r(Damgr_Paths *paths, const char *home);

Result_t(Nullptr_t, ErrMessage_t)
    damgr_resolve_host_path(const Damgr_Paths *paths, char *fid_buffer,
                            size_t size, const char *host_name, bool is_state);

Result_t(Nullptr_t, ErrMessage_t)
    damgr_resolve_module_path(const Damgr_Paths *paths, char *fid_buffer,
                              size_t size, const char *module_name,
                              bool is_state);

#ifdef DAMGR_PATHS_IMPLEMENTATION

Result_t(Nullptr_t, ErrMessage_t)
    damgr_paths_init_r(Damgr_Paths *paths, const char *home) {
  Result_t(Nullptr_t, ErrMessage_t) nullptr_r;
  nullptr_r = snprintf_r(paths->home, sizeof(paths->home), "%s", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r = snprintf_r(paths->active_dir, sizeof(paths->active_dir),
                         "%s/.config/damgr", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r = snprintf_r(paths->active_config, sizeof(paths->active_config),
                         "%s/.config/damgr/config.conf", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r = snprintf_r(paths->active_host_dir, sizeof(paths->active_host_dir),
                         "%s/.config/damgr/hosts", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r =
      snprintf_r(paths->active_module_dir, sizeof(paths->active_module_dir),
                 "%s/.config/damgr/modules", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r =
      snprintf_r(paths->active_dotfile_dir, sizeof(paths->active_dotfile_dir),
                 "%s/.config/damgr/dotfiles", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r = snprintf_r(paths->state_dir, sizeof(paths->state_dir),
                         "%s/.local/state/damgr", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r = snprintf_r(paths->state_config, sizeof(paths->state_config),
                         "%s/.local/state/damgr/state_config.conf", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r = snprintf_r(paths->staging_dir, sizeof(paths->staging_dir),
                         "%s/.local/state/damgr/.staging", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r = snprintf_r(paths->backup_dir, sizeof(paths->backup_dir),
                         "%s/.local/state/damgr/.backup", home);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r = snprintf_r(paths->journal, sizeof(paths->journal),
                         "%s/.local/state/damgr/.journal", home);
  if (!nullptr_r.is_success)
    return nullptr_r;

  return nullptr_r;
}

static Result_t(Nullptr_t, ErrMessage_t)
    append_conf(const char *path, const char *name, char *fid_buffer,
                size_t size, bool is_state) {
  if (is_state)
    return snprintf_r(fid_buffer, size, "%s/state_%s.conf", path, name);

  return snprintf_r(fid_buffer, size, "%s/%s.conf", path, name);
}

Result_t(Nullptr_t, ErrMessage_t)
    damgr_resolve_host_path(const Damgr_Paths *paths, char *fid_buffer,
                            size_t size, const char *host_name, bool is_state) {
  Result_t(Nullptr_t, ErrMessage_t) nullptr_r;
  if (is_state) {
    nullptr_r =
        append_conf(paths->state_dir, host_name, fid_buffer, size, is_state);
  } else {
    nullptr_r = append_conf(paths->active_host_dir, host_name, fid_buffer, size,
                            is_state);
  }
  return nullptr_r;
}

Result_t(Nullptr_t, ErrMessage_t)
    damgr_resolve_module_path(const Damgr_Paths *paths, char *fid_buffer,
                              size_t size, const char *module_name,
                              bool is_state) {
  Result_t(Nullptr_t, ErrMessage_t) nullptr_r;
  if (is_state) {
    nullptr_r =
        append_conf(paths->state_dir, module_name, fid_buffer, size, is_state);
  } else {
    nullptr_r = append_conf(paths->active_module_dir, module_name, fid_buffer,
                            size, is_state);
  }
  return nullptr_r;
}
#endif /* DAMGR_PATHS_IMPLEMENTATION */
#endif /* DAMGR_PATHS_H */
