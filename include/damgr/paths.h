#ifndef DAMGR_PATHS_H
#define DAMGR_PATHS_H
#include <stdio.h>

#define DAMGR_FID_BUF_MAX 4096

typedef struct paths {
  char home[DAMGR_FID_BUF_MAX];
  char active_dir[DAMGR_FID_BUF_MAX];         // ~/.config/damgr
  char active_config[DAMGR_FID_BUF_MAX];      // ~/.config/damgr/config.conf
  char active_host_dir[DAMGR_FID_BUF_MAX];    // ~/.config/damgr/hosts
  char active_module_dir[DAMGR_FID_BUF_MAX];  // ~/.config/damgr/modules
  char active_dotfile_dir[DAMGR_FID_BUF_MAX]; // ~/.config/damgr/dotfiles
  char state_dir[DAMGR_FID_BUF_MAX];          // ~/.local/state/damgr
  char staging_dir[DAMGR_FID_BUF_MAX];        // ~/.local/state/damgr/.staging
  char backup_dir[DAMGR_FID_BUF_MAX];         // ~/.local/state/damgr/.backup
  char journal[DAMGR_FID_BUF_MAX];            // ~/.local/state/damgr/.journal
} Damgr_Paths;

bool damgr_paths_init(Damgr_Paths *paths, const char *home);

void damgr_paths_active_host_conf(const Damgr_Paths *paths, const char *name,
                                  char *out, size_t out_size);

void damgr_paths_active_module_conf(const Damgr_Paths *paths, const char *name,
                                    char *out, size_t out_size);

void damgr_paths_state_conf(const Damgr_Paths *paths, const char *name,
                            char *out, size_t out_size);

void damgr_paths_staging_conf(const Damgr_Paths *paths, const char *name,
                              char *out, size_t out_size);

void damgr_paths_dotfile_src(const Damgr_Paths *paths, const char *name,
                             char *out, size_t out_size);

void damgr_paths_dotfile_dst(const Damgr_Paths *paths, const char *name,
                             char *out, size_t out_size);

#endif /* DAMGR_PATHS_H */
