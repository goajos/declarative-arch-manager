#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../context.h"

void execute_package_install_command(bool privileged, char *command, char *fid, Packages packages) {
    printf("Executing the package install command...\n");
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
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

void execute_package_remove_command(char *command, Packages packages) {
    printf("Executing the package remove command...\n");
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", command, "-Rs", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (int i = 0; i < (int)packages.count; ++i) {
            Package package = packages.items[i];
            if (!package.active) {
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
    
    String aur_helper = context.aur_helper;
    char fid[path_max];
    snprintf(fid, sizeof(fid), "/usr/bin/%s", aur_helper.data);
   
    // TODO: not required?
    // context = get_installed_packages(context);

    execute_package_remove_command("pacman", context.packages);
    execute_package_remove_command("pacman", context.aur_packages);
    
    execute_package_install_command(true, "pacman", "/usr/bin/pacman", context.packages);
    execute_package_install_command(false, aur_helper.data, fid, context.aur_packages);


    return context; 
}

