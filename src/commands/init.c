#include "command_utils.h"

int damgr_init() {
    puts("hello from damgr init...");
    int ret;
    struct stat st;
    char* src = "/usr/share/damgr";
    char fidbuf[path_max];
    // create the .local/share/damgr folder
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/state/damgr", get_user());
    // stat returns -1 if fidbuf doesn't exist
    if (stat(fidbuf, &st) == -1) mkdir(fidbuf, 0777);

    // create the .config/damgr folder
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damgr", get_user());
    // stat returns -1 if fidbuf doesn't exist
    if (stat(fidbuf, &st) == -1) mkdir(fidbuf, 0777);
    else {
        puts("Config already exists:");
        puts("Remove the .config/damgr folder before calling init again");
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    ret = recursive_init_state(st, src, fidbuf);

    exit_cleanup:
    return ret;
}
