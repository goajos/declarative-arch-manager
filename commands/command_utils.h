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
//TODO: finish these commands
int execute_dotfile_unsync_command();
int execute_dotfile_sync_command();
int execute_hook_command(bool privileged,
                        char* hook);

#endif /* COMMAND_UTILS_H */
