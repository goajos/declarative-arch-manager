#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include "../context.h"

void execute_package_install_command(Packages packages) {
    printf("Executing the package install command...\n");
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", "pacman", "-Syu", "--needed", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (int i = 0; i < (int)packages.count; ++i) {
            Package package = packages.items[i];
            if (package.active) {
                write(pipe_fds[1], package.item.data, package.item.len); 
                write(pipe_fds[1], "\n", 1);
            }
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
}

void execute_aur_package_install_command(char* path, char* command, Packages aur_packages) {
    printf("Executing the aur package install command...\n");
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl(path, command, "-Syu", "--needed", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (int i = 0; i < (int)aur_packages.count; ++i) {
            Package package = aur_packages.items[i];
            if (package.active) {
                write(pipe_fds[1], package.item.data, package.item.len); 
                write(pipe_fds[1], "\n", 1);
            }
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
}

Context damngr_sync(Context context)
{
    printf("hello from inside damngr_sync...\n");

    context = get_context(context);
    
    context = get_installed_packages(context);

    execute_package_install_command(context.packages);

    String aur_helper = context.aur_helper;
    char fid[0x4096]; // TODO: magic number for max path length?
    snprintf(fid, sizeof(fid), "/usr/bin/%s", aur_helper.data);
    execute_aur_package_install_command(fid, aur_helper.data, context.aur_packages);

    return context; 
}

