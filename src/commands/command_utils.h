#ifndef  COMMAND_UTILS_H
#define  COMMAND_UTILS_H
#include <stdlib.h>
#include <sys/stat.h>
#include "../state/state.h"

char* get_user();

int copy_file(char* src, char* dst);
int recursive_init_state(struct stat st, char* src, char* dst);

int execute_package_remove_command(struct dynamic_array packages);
int execute_package_install_command(struct dynamic_array packages);
int execute_aur_package_install_command(char* fid,
                                        char* command,
                                        struct dynamic_array aur_packages);
int execute_service_command(bool privileged,
                            bool enable,
                            struct dynamic_array services);
int execute_dotfile_link_command(bool link, struct dynamic_array dotfiles);
int execute_hook_command(bool privileged, struct dynamic_array hooks);

int execute_aur_update_command(char* fid, char* aur_helper);
int execute_update_command();

#endif /* COMMAND_UTILS_H */
