#ifndef DAMGR_CMD_H
#define DAMGR_CMD_H
#include "damgr/log.h"
#include "damgr/paths.h"
#include "damgr/utils.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int execute_execv(const char *fid_buf, char *const argv[]);
int run_package_command(const bool install, char *const *names,
                        const size_t cnt);
int run_aur_install_command(const char *helper, char *const *names,
                            const size_t cnt);
int run_service_command(const bool privileged, const bool to_enable,
                        const char *service);
int do_dotfile_link(const Damgr_Paths *paths, const char *dotfile);
int do_dotfile_unlink(const Damgr_Paths *paths, const char *dotfile);

#endif /* DAMGR_CMD_H */
