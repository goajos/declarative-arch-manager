#include <stdio.h>
#include <stdlib.h>
#include "../state/state.h"
#include "utils.c"

int damngr_merge() {
    puts("hello from damngr merge...");
    int ret;

    char fidbuf[path_max];
    struct config old_config = { }; 
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/share/damngr/config.kdl", get_user());
    FILE* old_config_fid = fopen(fidbuf, "r");
    if (old_config_fid != nullptr) { 
        // if old state exists, parse the old state config.kdl
        ret = parse_config_kdl(old_config_fid, &old_config);
        fclose(old_config_fid);
        if (ret == EXIT_FAILURE) return ret;
    }

    struct config new_config = { };
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/config.kdl", get_user());
    FILE* new_config_fid = fopen(fidbuf, "r");
    // always parse the new state config.kdl
    ret = parse_config_kdl(new_config_fid, &new_config);
    fclose(new_config_fid);
    if (ret == EXIT_FAILURE) return ret;


    struct host* active_host;
    if (old_config_fid != nullptr) { 
        // if the old state exists, parse the old states active host.kdl
        active_host = &old_config.active_host;
        snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/share/damngr/hosts/%s_state.kdl", get_user(), active_host->name);
        FILE* old_host_fid = fopen(fidbuf, "r");
        ret = parse_host_kdl(old_host_fid, active_host);
        fclose(old_host_fid);
        if (ret == EXIT_FAILURE) return ret;
    } 

    active_host = &new_config.active_host;
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/hosts/%s.kdl", get_user(), active_host->name);
    FILE* new_host_fid = fopen(fidbuf, "r");
    // always parse the new states active host.kdl
    ret = parse_host_kdl(new_host_fid, active_host);
    fclose(new_host_fid);
    if (ret == EXIT_FAILURE) return ret;
    
    struct modules* modules;
    struct module* module;
    if (old_config_fid != nullptr) {
        // if the old state exists, parse the old states modules module.kdl (1 parse/module)
        modules = &old_config.active_host.modules;
        for (size_t i = 0; i < modules->count; ++i) {
            module = &modules->items[i];
            snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/share/damngr/modules/%s_state.kdl", get_user(), module->name);
            FILE* old_module_fid = fopen(fidbuf, "r");
            ret = parse_module_kdl(old_module_fid, module);
            fclose(old_module_fid);
            if (ret == EXIT_FAILURE) return ret; 
        }
    }

    // always parse the new states modules module.kdl (1 parse/module)
    modules = &new_config.active_host.modules;
    for (size_t i = 0; i < modules->count; ++i) {
        module = &modules->items[i];
        snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/modules/%s.kdl", get_user(), module->name);
        FILE* new_module_fid = fopen(fidbuf, "r");
        ret = parse_module_kdl(new_module_fid, module);
        fclose(new_module_fid);
        if (ret == EXIT_FAILURE) return ret; 
    }

    return ret;
}
