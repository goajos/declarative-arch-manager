#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "utils.h"

int damngr_init() {
    puts("hello from damngr init...");
    int ret;
    struct stat st;
    [[maybe_unused]] char* src = "damngr";
    char fidbuf[path_max]; 
    // create the .local/share/damngr folder
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/share/damngr", get_user());
    // stat returns -1 if fidbuf doesn't exist
    if (stat(fidbuf, &st) == 1) mkdir(fidbuf, 0777);

    // create the .config/damngr folder
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr", get_user());
    // stat returns -1 if fidbuf doesn't exist
    if (stat(fidbuf, &st) == 1) mkdir(fidbuf, 0777);
    else {
        puts("Config already exists:");
        puts("Remove the .config/damngr folder before calling init again");
        ret = EXIT_FAILURE;
        goto exit_cleanup; 
    }
    // ret = recursive_init_state_copy(st, src, fidbuf);

    exit_cleanup:
    return ret;
}
