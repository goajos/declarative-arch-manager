#define _DEFAULT_SOURCE
#include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <glob.h>
// #include "utils/parse.c"
// #include "utils/debug.c"
#include "commands/init.c"
#include "commands/validate.c"
#include "commands/sync.c"
#include "commands/update.c"
#include "utils/string.c"


int main(int argc, char *argv[])
{
    if (argc == 1 || argc > 2) {
        printf("Print help...\n"); // TODO: wrong amount of cli arguments
        return 0;
    }
    
    int command_idx;
    if (string_equal(String(argv[1]), String("init"))) command_idx = 0; 
    else if (string_equal(String(argv[1]), String("validate"))) command_idx = 1;
    else if (string_equal(String(argv[1]), String("sync"))) command_idx = 2;
    else if (string_equal(String(argv[1]), String("update"))) command_idx = 3;
    else {
        printf("Print help...\n"); // TODO: unsupported command argument
        return 0;
    }

    switch(command_idx) {
        case 0:
            printf("damngr init called...\n");
            damngr_init();
            break;
        case 1:
            printf("damngr validate called...\n");
            damngr_validate();
            break;
        case 2:
            printf("damngr sync called...\n");
            damngr_sync();
            break;
        case 3:
            printf("damngr update called...\n");
            damngr_update();
            break;
    }

    return 0;
}

// clang main.c -o main -g -lc -lkdl -lm -std=c23 -Wall -Wextra -Werror -pedantic
// int main()
// {
//     Hosts hosts = {0};
//
//     FILE *config_fid = fopen("/home/jappe/.config/damngr/config.kdl", "r");
//     parse_config_kdl(config_fid, &hosts);
//     fclose(config_fid);
//
//     printf("hosts count: %d\n", (int)hosts.count);
//     String active_host = { .data = nullptr };
//     for (int i = 0; i < (int)hosts.count; ++i) {
//         if (hosts.items[i].active) {
//            active_host = String(hosts.items[i].item.data);
//            break; // only 1 active host assumption
//         }
//     }
//     // TODO: no active host exit
//     printf("active host: %s\n", active_host.data);
//
//     glob_t host_globbuf;
//     host_globbuf.gl_offs = 0; // not prepending any flags to glob
//     glob("/home/jappe/.config/damngr/hosts/*.kdl", GLOB_DOOFFS, NULL, &host_globbuf);
//     String host_path = { .data = nullptr };
//     for (size_t i = 0; i < host_globbuf.gl_pathc; ++i) {
//         String glob_path = String(host_globbuf.gl_pathv[i]);
//         if (string_contains(glob_path, active_host)) {
//             host_path = string_copy(glob_path);
//             break; // only 1 active host assumption
//         }
//     }
//
//     Modules modules = {0};
//     Services services = {0};
//     Hooks hooks = {0};
//     // if an active host is found
//     if (host_path.data != nullptr) {
//         char fidbuf[0x4096]; // TODO: magic number for max path length? 
//         printf("%s\n", host_path.data);
//         snprintf(fidbuf, sizeof(fidbuf), "%s", host_path.data);
//         FILE *host_fid = fopen(fidbuf, "r");  
//         parse_host_kdl(host_fid, &modules, &services, &hooks);
//         fclose(host_fid);
//
//         debug_active(modules)
//     } else return 1;
//     // no longer need hosts and host glob
//     memory_cleanup(hosts);
//     free_sized(host_path.data, host_path.len);
//     globfree(&host_globbuf);
//
//     Packages packages = {0};
//     Packages aur_packages = {0};
//     for (int i = 0; i < (int)modules.count; ++i) {
//         Module module = modules.items[i];
//         if (module.active) {
//             char fidbuf[0x4096]; // TODO: magic number for max path length? 
//             snprintf(fidbuf, sizeof(fidbuf), "/home/jappe/.config/damngr/modules/%s.kdl", module.item.data);
//             printf("%s\n", fidbuf);
//             FILE *module_fid = fopen(fidbuf, "r");
//             parse_module_kdl(module_fid, &packages, &aur_packages, &services, &hooks);
//             fclose(module_fid);
//         }
//
//     }
//     debug_active(packages);
//     debug_active(aur_packages);
//     debug_active_user_type(services);
//
//     execute_package_install_command(packages);
//     // TODO: read aur helper from config?
//     String aur_helper = String("paru");
//     char fid[0x4096]; // TODO: magic number for max path length?
//     snprintf(fid, sizeof(fid), "/usr/bin/%s", aur_helper.data);
//     execute_aur_package_install_command(fid, aur_helper.data, aur_packages);
//
//     memory_cleanup(modules);
//     memory_cleanup(packages);
//     memory_cleanup(services);
//     memory_cleanup(hooks);
//
//     return 0;
// }
