#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include "damgr/state.h"
#include <stdlib.h>

extern const int damgr_path_max;

int damgr_is_state_dir_empty(char *dir);
int damgr_init_dir(char *user, bool is_state);

char *damgr_get_user();

char *damgr_string_copy(char *str);

bool damgr_string_contains(char *haystack, char *needle);

void damgr_string_trim(char *str);

int damgr_get_conf_key(char *key);

int damgr_qcharcmp(const void *p1, const void *p2);

int damgr_execute_aur_update_command(char *aur_helper);
int damgr_execute_update_command();

int damgr_execute_package_install_command(struct darray packages);
int damgr_execute_aur_package_install_command(struct darray packages,
                                              char *aur_helper);
int damgr_execute_package_remove_command(struct darray packages);
// TODO: add a check to see if the shell commands are executable or not
// TODO: can the shell commands output be supressed?
int damgr_execute_hook_command(char *user, bool privileged, char *hook);
int damgr_execute_service_command(bool privileged, bool to_enable,
                                  char *service);
int damgr_execute_dotfile_command(char *user, bool to_link, char *service);

#endif /* DAMGR_UTILS_H */
