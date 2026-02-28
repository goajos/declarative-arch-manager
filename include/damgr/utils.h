#ifndef UTILS_H
#define UTILS_H
#include "damgr/state.h"
#include <stdlib.h>

constexpr int PATH_MAX = 4096;

int is_damgr_state_dir_empty(char *dir);

char *get_user();

size_t read_func(void *user_data, char *buf, size_t bufsize);

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

#endif /* UTILS_H */
