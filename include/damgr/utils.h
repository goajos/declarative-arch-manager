#ifndef UTILS_H
#define UTILS_H
#include "damgr/actions.h"
#include "damgr/state.h"
#include <stdlib.h>

constexpr int PATH_MAX = 4096;

int is_damgr_state_dir_empty(char *dir);
int init_damgr_state_dir();
int init_damgr_config_dir();

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
int execute_hook_command(bool privileged, char *hook);
int execute_service_command(bool privileged, bool to_enable, char *service);
int execute_dotfile_command(bool to_link, char *service);

int execute_aur_update_command(char *aur_helper);
int execute_update_command();

void free_config(struct config config);
void free_actions(struct actions actions);

#endif /* UTILS_H */
