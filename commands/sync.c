#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../context/context.h"
#include "../utils/array.h"
#include "../utils/memory.h"

[[maybe_unused]] static int execute_package_remove_command(char *command, struct active_dynamic_array packages) {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", command, "-Rs", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (size_t i = 0; i < packages.count; ++i) {
            struct active_element package = packages.elements[i];
            if (!package.active) {
                write(pipe_fds[1], package.element, strlen(package.element)); 
                write(pipe_fds[1], "\n", 1);
            }
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

[[maybe_unused]] static int execute_package_install_command(bool privileged, char *command, char *fid, struct active_dynamic_array packages) {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        if (privileged) {
            execl("/usr/bin/sudo", "sudo", command, "-S", "--needed", "-", nullptr);
        } else {
            execl(fid, command, "-S", "--needed", "-", nullptr);
        }
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (size_t i = 0; i < packages.count; ++i) {
            struct active_element package = packages.elements[i];
            if (package.active) {
                write(pipe_fds[1], package.element, strlen(package.element)); 
                write(pipe_fds[1], "\n", 1);
            }
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

int damngr_sync() {
    int ret;
    char* aur_helper = nullptr;
    struct active_dynamic_array packages = { };
    struct active_dynamic_array aur_packages = { };
    struct active_dynamic_array root_services = { };
    struct active_dynamic_array root_hooks = { };
    struct active_dynamic_array user_services = { };
    struct active_dynamic_array user_hooks = { };
    ret = get_sync_context(&aur_helper, &packages, &aur_packages, &root_services, &root_hooks, &user_services, &user_hooks);
    if (ret == EXIT_FAILURE) return ret;

    for (size_t i = 0; i < packages.count; ++i) {
        printf("%s: is %b\n", packages.elements[i].element, packages.elements[i].active);
    }
    for (size_t i = 0; i < aur_packages.count; ++i) {
        printf("%s: is %b\n", aur_packages.elements[i].element, aur_packages.elements[i].active);
    }
    
    // ret = execute_package_remove_command("pacman", packages);
    // if (ret == EXIT_FAILURE) return ret;
    // ret = execute_package_remove_command("pacman", aur_packages);
    // if (ret == EXIT_FAILURE) return ret;
    //
    // ret = execute_package_install_command(true, "pacman", "/usr/bin/pacman", packages);
    // if (ret == EXIT_FAILURE) return ret;
    // char fidbuf[path_max];
    // snprintf(fidbuf, sizeof(fidbuf), "/usr/bin/%s", aur_helper);
    // ret = execute_package_install_command(false, aur_helper, fidbuf, aur_packages);
    // if (ret == EXIT_FAILURE) return ret;

    // TODO: sync dotfiles
    // TODO: disable services
    // TODO: enable services
    // TODO: run post hooks

    free_sized(aur_helper, strlen(aur_helper));
    DYNAMIC_ARRAY_FREE(packages);
    DYNAMIC_ARRAY_FREE(aur_packages);
    DYNAMIC_ARRAY_FREE(root_services);
    DYNAMIC_ARRAY_FREE(root_hooks);
    DYNAMIC_ARRAY_FREE(user_services);
    DYNAMIC_ARRAY_FREE(user_hooks);

    return ret;
}

