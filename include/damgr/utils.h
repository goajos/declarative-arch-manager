#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include "damgr/actions.h"
#include "damgr/state.h"
#include <stdlib.h>

static const int PATH_MAX = 4096;

int is_damgr_state_dir_empty(char *dir);
int init_damgr_dir(char *user, bool is_state);

char *get_user();

size_t read_func(void *user_data, char *buf, size_t bufsize);
size_t write_func(void *user_data, char const *buf, size_t bufsize);

char *string_copy(char *str);
bool string_contains(char *haystack, char *needle);

int qcharcmp(const void *p1, const void *p2);

int execute_package_install_command(struct darray packages);
int execute_aur_package_install_command(struct darray packages,
                                        char *aur_helper);
int execute_package_remove_command(struct darray packages);
// TODO: add a check to see if the shell commands are executable or not
// TODO: can the shell commands output be supressed?
int execute_hook_command(char *user, bool privileged, char *hook);
int execute_service_command(bool privileged, bool to_enable, char *service);
int execute_dotfile_command(char *user, bool to_link, char *service);

int execute_aur_update_command(char *aur_helper);
int execute_update_command();

void log_module_actions(struct module module, bool is_state);

void free_config(struct config config);
void free_actions(struct actions actions);

#endif /* DAMGR_UTILS_H */
