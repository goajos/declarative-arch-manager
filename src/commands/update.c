#include "command_utils.h"

int damngr_update() {
    puts("hello from damngr update...");
    int ret;
    
    char fidbuf[path_max];
    struct config config = { };
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/config.kdl", get_user());
    FILE* config_fid = fopen(fidbuf, "r");
    if (config_fid == nullptr) {
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    ret = parse_config_kdl(config_fid, &config);
    fclose(config_fid);
    if (ret == EXIT_FAILURE) goto exit_cleanup;
    
    if (config.aur_helper) {
        snprintf(fidbuf, sizeof(fidbuf), "/usr/bin/%s", config.aur_helper);
        ret = execute_aur_update_command(fidbuf, config.aur_helper);
    } else ret = execute_update_command();

    exit_cleanup:
        free_config(config);
    return ret;
}
