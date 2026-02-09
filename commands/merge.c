#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../state/state.h"
#include "command_utils.h"

// static int execute_package_remove_command(struct dynamic_array packages) {
//     int pipe_fds[2];
//     pipe(pipe_fds);
//     pid_t pid = fork();
//     if (pid == -1) return EXIT_FAILURE;
//     if (pid == 0) {
//         close(pipe_fds[1]); // close pipe write end child
//         dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
//         close(pipe_fds[0]); // close pipe read end child
//         execl("/usr/bin/sudo", "sudo", "pacman", "-Rs", "-", nullptr);
//     } else {
//         close(pipe_fds[0]); // close pipe read end
//         for (size_t i = 0; i < packages.count; ++i) {
//             char* package = packages.items[i];
//             write(pipe_fds[1], package, strlen(package)); 
//             write(pipe_fds[1], "\n", 1);
//         }
//         close(pipe_fds[1]); // close pipe write end
//         waitpid(pid, nullptr, 0);
//     }
//     return EXIT_SUCCESS;
// }
//
// static int execute_package_install_command(bool privileged, char *fid, char *command, struct dynamic_array packages) {
//     int pipe_fds[2];
//     pipe(pipe_fds);
//     pid_t pid = fork();
//     if (pid == -1) return EXIT_FAILURE;
//     if (pid == 0) {
//         close(pipe_fds[1]); // close pipe write end child
//         dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
//         close(pipe_fds[0]); // close pipe read end child
//         if (privileged) {
//             execl("/usr/bin/sudo", "sudo", command, "-S", "--needed", "-", nullptr);
//         } else {
//             execl(fid, command, "-S", "--needed", "-", nullptr);
//         }
//     } else {
//         close(pipe_fds[0]); // close pipe read end
//         for (size_t i = 0; i < packages.count; ++i) {
//             char* package = packages.items[i];
//             write(pipe_fds[1], package, strlen(package)); 
//             write(pipe_fds[1], "\n", 1);
//         }
//         close(pipe_fds[1]); // close pipe write end
//         waitpid(pid, nullptr, 0);
//     }
//     return EXIT_SUCCESS;
// }
//
// // TODO: how to make atomic
// int handle_package_actions(struct package_actions package_actions,
//                         struct package_actions aur_package_actions,
//                         char* aur_helper) {
//     int ret;
//     if (package_actions.items != nullptr) {
//         struct dynamic_array to_install = { };
//         struct dynamic_array to_remove = { };
//         for (size_t i = 0; i < package_actions.count; ++i) {
//             struct package_action action = package_actions.items[0];
//             if (action.to_install) DYNAMIC_ARRAY_APPEND(to_install, action.name);
//             else DYNAMIC_ARRAY_APPEND(to_remove, action.name);
//         }
//         if (to_remove.count > 0) {
//             ret = execute_package_remove_command(to_remove);
//         }
//         if (to_install.count > 0) {
//             ret = execute_package_install_command(true, "/usr/bin/pacman", "pacman", to_install);
//         }
//     }
//     if (aur_package_actions.items != nullptr) {
//         struct dynamic_array to_install = { };
//         struct dynamic_array to_remove = { };
//         for (size_t i = 0; i < aur_package_actions.count; ++i) {
//             struct package_action action = aur_package_actions.items[0];
//             if (action.to_install) DYNAMIC_ARRAY_APPEND(to_install, action.name);
//             else DYNAMIC_ARRAY_APPEND(to_remove, action.name);
//         }
//         if (to_remove.count > 0) {
//             ret = execute_package_remove_command(to_remove);
//         }
//         if (to_install.count > 0) {
//             char fidbuf[path_max];
//             snprintf(fidbuf, sizeof(fidbuf), "/usr/bin/%s", aur_helper);
//             ret = execute_package_install_command(false, fidbuf, aur_helper, to_install);
//         }
//     }
//     return ret;
// }
//
// static int execute_service_command(bool privileged, bool enable, struct dynamic_array services) {
//     for (size_t i = 0; i < services.count; ++i) {
//         pid_t pid = fork();
//         if (pid == -1) return EXIT_FAILURE;
//         if (pid == 0) {
//             struct dynamic_array argv = { };
//             if (privileged) {
//                 DYNAMIC_ARRAY_APPEND(argv, "sudo");
//                 DYNAMIC_ARRAY_APPEND(argv, "systemctl");
//             }
//             else {
//                 DYNAMIC_ARRAY_APPEND(argv, "systemctl");
//                 DYNAMIC_ARRAY_APPEND(argv, "--user");
//             }
//             if (enable) DYNAMIC_ARRAY_APPEND(argv, "enable");
//             else DYNAMIC_ARRAY_APPEND(argv, "disable");
//             DYNAMIC_ARRAY_APPEND(argv, services.items[i]);
//             DYNAMIC_ARRAY_APPEND(argv, nullptr);
//             if (privileged) execv("/usr/bin/sudo", argv.items);
//             else execv("/usr/bin/systemctl", argv.items);
//         } else {
//             waitpid(pid, nullptr, 0);
//         }
//     }
//     return EXIT_SUCCESS;
// }
//
// // TODO: how to make atomic?
// int handle_service_actions(struct service_actions root_service_actions,
//                         struct service_actions user_service_actions) {
//     int ret;
//     if (root_service_actions.items != nullptr) {
//         struct dynamic_array to_enable = { };
//         struct dynamic_array to_disable = { };
//         for (size_t i = 0; i < root_service_actions.count; ++i) {
//             struct service_action action = root_service_actions.items[0];
//             if (action.to_enable) DYNAMIC_ARRAY_APPEND(to_enable, action.name);
//             else DYNAMIC_ARRAY_APPEND(to_disable, action.name);
//         }
//         if (to_disable.count > 0) {
//             ret = execute_service_command(true, false, to_disable);
//         }
//         if (to_enable.count > 0) {
//             ret = execute_service_command(true, true, to_enable);
//         }
//     }
//     if (user_service_actions.items != nullptr) {
//         struct dynamic_array to_enable = { };
//         struct dynamic_array to_disable = { };
//         for (size_t i = 0; i < user_service_actions.count; ++i) {
//             struct service_action action = user_service_actions.items[0];
//             if (action.to_enable) DYNAMIC_ARRAY_APPEND(to_enable, action.name);
//             else DYNAMIC_ARRAY_APPEND(to_disable, action.name);
//         }
//         if (to_disable.count > 0) {
//             ret = execute_service_command(true, false, to_disable);
//         }
//         if (to_enable.count > 0) {
//             ret = execute_service_command(true, true, to_enable);
//         }
//     }
//     return ret;
// }

// TODO: atomic operation? either everything works or full rollback? (atomic merge or write only?)
// TODO: merge should create a state-bak folder that gets deleted if succesful or restored otherwise
// TODO: rewrite all if else with { }?
int damngr_merge() {
    puts("hello from damngr merge...");
    int ret;

    char fidbuf[path_max];
    struct config old_config = { }; 
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/share/damngr/config_state.kdl", get_user());
    FILE* old_config_fid = fopen(fidbuf, "r");
    if (old_config_fid != nullptr) { 
        // if old state exists, parse the old state config.kdl
        ret = parse_config_kdl(old_config_fid, &old_config);
        fclose(old_config_fid);
        if (ret == EXIT_FAILURE) goto exit_cleanup;
    }

    struct config new_config = { };
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/config.kdl", get_user());
    FILE* new_config_fid = fopen(fidbuf, "r");
    if (new_config_fid == nullptr) {
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    // always parse the new state config.kdl
    ret = parse_config_kdl(new_config_fid, &new_config);
    fclose(new_config_fid);
    if (ret == EXIT_FAILURE) goto exit_cleanup;


    struct host* active_host;
    if (old_config_fid != nullptr) { 
        // if the old state exists, parse the old states active host.kdl
        active_host = &old_config.active_host;
        snprintf(fidbuf,
                sizeof(fidbuf),
                "/home/%s/.local/share/damngr/%s_state.kdl",
                get_user(),
                active_host->name);
        FILE* old_host_fid = fopen(fidbuf, "r");
        if (old_host_fid == nullptr) {
            ret = EXIT_FAILURE;
            goto exit_cleanup;
        }
        ret = parse_host_kdl(old_host_fid, active_host);
        fclose(old_host_fid);
        if (ret == EXIT_FAILURE) goto exit_cleanup;
    }

    active_host = &new_config.active_host;
    snprintf(fidbuf,
            sizeof(fidbuf),
            "/home/%s/.config/damngr/hosts/%s.kdl",
            get_user(),
            active_host->name);
    FILE* new_host_fid = fopen(fidbuf, "r");
    if (new_host_fid == nullptr) {
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    // always parse the new states active host.kdl
    ret = parse_host_kdl(new_host_fid, active_host);
    fclose(new_host_fid);
    if (ret == EXIT_FAILURE) goto exit_cleanup;
    
    struct modules* modules;
    struct module* module;
    if (old_config_fid != nullptr) {
        // if the old state exists, parse the old states modules module.kdl (1 parse/module)
        modules = &old_config.active_host.modules;
        for (size_t i = 0; i < modules->count; ++i) {
            module = &modules->items[i];
            snprintf(fidbuf,
                    sizeof(fidbuf),
                    "/home/%s/.local/share/damngr/%s_state.kdl",
                    get_user(),
                    module->name);
            FILE* old_module_fid = fopen(fidbuf, "r");
            if (old_module_fid == nullptr) {
                ret = EXIT_FAILURE;
                goto exit_cleanup;
            }
            ret = parse_module_kdl(old_module_fid, module);
            fclose(old_module_fid);
            if (ret == EXIT_FAILURE) goto exit_cleanup; 
        }
    }

    // always parse the new states modules module.kdl (1 parse/module)
    modules = &new_config.active_host.modules;
    for (size_t i = 0; i < modules->count; ++i) {
        module = &modules->items[i];
        snprintf(fidbuf,
                sizeof(fidbuf),
                "/home/%s/.config/damngr/modules/%s.kdl",
                get_user(),
                module->name);
        FILE* new_module_fid = fopen(fidbuf, "r");
        if (new_module_fid == nullptr) {
            ret = EXIT_FAILURE;
            goto exit_cleanup;
        }
        ret = parse_module_kdl(new_module_fid, module);
        fclose(new_module_fid);
        if (ret == EXIT_FAILURE) goto exit_cleanup; 
    }

    struct service_actions service_actions = { };
    struct package_actions package_actions = { };
    struct package_actions aur_package_actions = { };
    struct dotfile_actions dotfile_actions = { };
    struct hook_actions hook_actions = { };
    ret = determine_actions(&old_config,
                    &new_config,
                    &service_actions,
                    &package_actions,
                    &aur_package_actions,
                    &dotfile_actions,
                    &hook_actions);
    if (ret == EXIT_FAILURE) goto action_cleanup;
   
    // ret = handle_package_actions(package_actions, aur_package_actions, new_config.aur_helper);
    // ret = handle_service_actions(root_service_actions, user_service_actions);
    // ret = handle_dotfile_actions(dotfile_actions);
    // ret = handle_hook_actions(root_hook_actions, user_hook_actions);

    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/share/damngr/config_state.kdl", get_user());
    FILE* new_config_state_fid = fopen(fidbuf, "w");
    if (new_config_state_fid == nullptr) {
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    ret = write_config_kdl(new_config_state_fid, &new_config);
    fclose(new_config_state_fid);
    if (ret == EXIT_FAILURE) goto exit_cleanup; 
    snprintf(fidbuf,
            sizeof(fidbuf),
            "/home/%s/.local/share/damngr/%s_state.kdl",
            get_user(),
            active_host->name);
    FILE* new_host_state_fid = fopen(fidbuf, "w");
    if (new_host_state_fid == nullptr) {
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    ret = write_host_kdl(new_host_state_fid, active_host);
    if (ret == EXIT_FAILURE) goto exit_cleanup; 
    fclose(new_host_state_fid);
    FILE* new_module_state_fid;
    for (size_t i = 0; i < modules->count; ++i) {
        snprintf(fidbuf,
                sizeof(fidbuf),
                "/home/%s/.local/share/damngr/%s_state.kdl",
                get_user(),
                modules->items[i].name);
        new_module_state_fid = fopen(fidbuf, "w");
        if (new_module_state_fid == nullptr) {
            ret = EXIT_FAILURE;
            goto exit_cleanup;
        }
        ret = write_module_kdl(new_module_state_fid, &modules->items[i]);
        fclose(new_module_state_fid);
        if (ret == EXIT_FAILURE) goto exit_cleanup; 
    }

    action_cleanup:
        free_actions(service_actions,
                    package_actions,
                    aur_package_actions,
                    dotfile_actions,
                    hook_actions);

    exit_cleanup:
        if (old_config_fid != nullptr) {    
            free_config(old_config);
        } 
        free_config(new_config);
    return ret;
}
