#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../context.h"

void execute_update_command() {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", "pacman", "-Syu", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
}

void execute_aur_update_command(char* path, char* command) {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl(path, command, "-Syu", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
}

Context damngr_update(Context context)
{
    context = get_context(context);

    String aur_helper = context.aur_helper;
    if (aur_helper.data != nullptr) {
        char fid[path_max];
        snprintf(fid, sizeof(fid), "/usr/bin/%s", aur_helper.data);
        execute_aur_update_command(fid, aur_helper.data);
    } else execute_update_command();

    return context; 
}

