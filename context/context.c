#include "context.h"
#include "utils/kdl_parse.c"
#include "utils/user.c"

int get_aur_helper(char** aur_helper) {
    char fidbuf[path_max];
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/config.kdl", get_user());
    FILE* config_fid = fopen(fidbuf, "r");
    int ret = parse_config_kdl(config_fid, nullptr, aur_helper);
    fclose(config_fid);
    return ret;
}
