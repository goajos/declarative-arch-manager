#ifndef  COMMAND_UTILS_H
#define  COMMAND_UTILS_H
#include <stdlib.h>
#include "../state/state.h"

char* get_user();

int execute_package_remove_command(struct dynamic_array packages);
int execute_package_install_command(struct dynamic_array packages);
int execute_aur_package_install_command(char* fid,
                                        char* command,
                                        struct dynamic_array aur_packages);
int execute_service_command(bool privileged,
                            bool eneable,
                            struct dynamic_array services);

#endif /* COMMAND_UTILS_H */
