#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../context/context.h"
#include "../utils/memory.h"

static int execute_update_command() {
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) { 
        execl("/usr/bin/sudo", "sudo", "pacman", "-Syu", nullptr);
    } else {
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

static int execute_aur_update_command(char* path, char* command) {
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        execl(path, command, "-Syu", nullptr);
    } else {
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}


int damngr_update() {
    int ret;
    char* aur_helper = nullptr;
    ret = get_aur_helper(&aur_helper);
    if (ret == EXIT_FAILURE) return ret;

    if (aur_helper) {
       char fidbuf[path_max];
       snprintf(fidbuf, sizeof(fidbuf), "/usr/bin/%s", aur_helper);
       ret = execute_aur_update_command(fidbuf, aur_helper);
    } else ret = execute_update_command();

    free_sized(aur_helper, strlen(aur_helper));

    return ret;
}

