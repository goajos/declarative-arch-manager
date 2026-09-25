#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../state/state.h"
 
char* get_user() {
    struct passwd* pwd = getpwuid(geteuid());
    return pwd->pw_name;
}

int execute_package_remove_command(struct dynamic_array packages) {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", "pacman", "-Rs", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (size_t i = 0; i < packages.count; ++i) {
            char* package = packages.items[i];
            write(pipe_fds[1], package, strlen(package)); 
            write(pipe_fds[1], "\n", 1);
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

int execute_package_install_command(struct dynamic_array packages) {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", "pacman", "-S", "--needed", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (size_t i = 0; i < packages.count; ++i) {
            char* package = packages.items[i];
            write(pipe_fds[1], package, strlen(package)); 
            write(pipe_fds[1], "\n", 1);
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

int execute_aur_package_install_command(char* fid,
                                        char* command,
                                        struct dynamic_array packages) {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl(fid, command, "-S", "--needed", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (size_t i = 0; i < packages.count; ++i) {
            char* package = packages.items[i];
            write(pipe_fds[1], package, strlen(package)); 
            write(pipe_fds[1], "\n", 1);
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

int execute_service_command(bool privileged, bool enable, struct dynamic_array services) {
    for (size_t i = 0; i < services.count; ++i) {
        pid_t pid = fork();
        if (pid == -1) return EXIT_FAILURE;
        if (pid == 0) {
            if (privileged) {
                char* argv[] = { "sudo",
                            "systemctl",
                            (enable) ? "enable" : "disable",
                            services.items[i],
                            nullptr };
                execv("/usr/bin/sudo", argv);
            }
            else {
                char* argv[] = { "systemctl",
                            "--user",
                            (enable) ? "enable" : "disable",
                            services.items[i],
                            nullptr };
                execv("/usr/bin/systemctl", argv);
            }
        } else {
            waitpid(pid, nullptr, 0);
        }
    }
    return EXIT_SUCCESS;
}
