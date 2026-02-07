#include <stdio.h>
#include <stdlib.h>
#include "../state/state.h"
#include "command_utils.h"


// TODO: atomic operation? either everything works or full rollback? (atomic merge or write only?)
// TODO: merge should create a state-bak folder that gets deleted if succesful or restored otherwise
int damngr_merge() {
    puts("hello from damngr merge...");
    int ret;

    char fidbuf[path_max];
    struct config old_config = { }; 
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/share/damngr/config_state.kdl", get_user());
    FILE* old_config_fid = fopen(fidbuf, "r");
    if (old_config_fid != nullptr) { 
        // if old state exists, parse the old state config.kdl
        ret = parse_config_kdl(old_config_fid, &old_config);
        fclose(old_config_fid);
        if (ret == EXIT_FAILURE) goto exit_cleanup;
    }

    struct config new_config = { };
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/config.kdl", get_user());
    FILE* new_config_fid = fopen(fidbuf, "r");
    if (new_config_fid == nullptr) {
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    // always parse the new state config.kdl
    ret = parse_config_kdl(new_config_fid, &new_config);
    fclose(new_config_fid);
    if (ret == EXIT_FAILURE) goto exit_cleanup;


    struct host* active_host;
    if (old_config_fid != nullptr) { 
        // if the old state exists, parse the old states active host.kdl
        active_host = &old_config.active_host;
        snprintf(fidbuf,
                sizeof(fidbuf),
                "/home/%s/.local/share/damngr/%s_state.kdl",
                get_user(),
                active_host->name);
        FILE* old_host_fid = fopen(fidbuf, "r");
        if (old_host_fid == nullptr) {
            ret = EXIT_FAILURE;
            goto exit_cleanup;
        }
        ret = parse_host_kdl(old_host_fid, active_host);
        fclose(old_host_fid);
        if (ret == EXIT_FAILURE) goto exit_cleanup;
    }

    active_host = &new_config.active_host;
    snprintf(fidbuf,
            sizeof(fidbuf),
            "/home/%s/.config/damngr/hosts/%s.kdl",
            get_user(),
            active_host->name);
    FILE* new_host_fid = fopen(fidbuf, "r");
    if (new_host_fid == nullptr) {
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    // always parse the new states active host.kdl
    ret = parse_host_kdl(new_host_fid, active_host);
    fclose(new_host_fid);
    if (ret == EXIT_FAILURE) goto exit_cleanup;
    
    struct modules* modules;
    struct module* module;
    if (old_config_fid != nullptr) {
        // if the old state exists, parse the old states modules module.kdl (1 parse/module)
        modules = &old_config.active_host.modules;
        for (size_t i = 0; i < modules->count; ++i) {
            module = &modules->items[i];
            snprintf(fidbuf,
                    sizeof(fidbuf),
                    "/home/%s/.local/share/damngr/%s_state.kdl",
                    get_user(),
                    module->name);
            FILE* old_module_fid = fopen(fidbuf, "r");
            if (old_module_fid == nullptr) {
                ret = EXIT_FAILURE;
                goto exit_cleanup;
            }
            ret = parse_module_kdl(old_module_fid, module);
            fclose(old_module_fid);
            if (ret == EXIT_FAILURE) goto exit_cleanup; 
        }
    }

    // always parse the new states modules module.kdl (1 parse/module)
    modules = &new_config.active_host.modules;
    for (size_t i = 0; i < modules->count; ++i) {
        module = &modules->items[i];
        snprintf(fidbuf,
                sizeof(fidbuf),
                "/home/%s/.config/damngr/modules/%s.kdl",
                get_user(),
                module->name);
        FILE* new_module_fid = fopen(fidbuf, "r");
        if (new_module_fid == nullptr) {
            ret = EXIT_FAILURE;
            goto exit_cleanup;
        }
        ret = parse_module_kdl(new_module_fid, module);
        fclose(new_module_fid);
        if (ret == EXIT_FAILURE) goto exit_cleanup; 
    }


    // TODO: calculate the diff to perform the merge step
    // use the diff to perform actions
    //      - remove packages
    //      - install packages
    //      - (de)sync dotfiles
    //      - disable services
    //      - enable services
    //      - run post hooks
    if (old_config_fid != nullptr) {
        ret = calculate_config_diff(&old_config, &new_config);
    }

    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/share/damngr/config_state.kdl", get_user());
    FILE* new_config_state_fid = fopen(fidbuf, "w");
    if (new_config_state_fid == nullptr) {
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    ret = write_config_kdl(new_config_state_fid, &new_config);
    fclose(new_config_state_fid);
    if (ret == EXIT_FAILURE) goto exit_cleanup; 
    snprintf(fidbuf,
            sizeof(fidbuf),
            "/home/%s/.local/share/damngr/%s_state.kdl",
            get_user(),
            active_host->name);
    FILE* new_host_state_fid = fopen(fidbuf, "w");
    if (new_host_state_fid == nullptr) {
        ret = EXIT_FAILURE;
        goto exit_cleanup;
    }
    ret = write_host_kdl(new_host_state_fid, active_host);
    if (ret == EXIT_FAILURE) goto exit_cleanup; 
    fclose(new_host_state_fid);
    FILE* new_module_state_fid;
    for (size_t i = 0; i < modules->count; ++i) {
        snprintf(fidbuf,
                sizeof(fidbuf),
                "/home/%s/.local/share/damngr/%s_state.kdl",
                get_user(),
                modules->items[i].name);
        new_module_state_fid = fopen(fidbuf, "w");
        if (new_module_state_fid == nullptr) {
            ret = EXIT_FAILURE;
            goto exit_cleanup;
        }
        ret = write_module_kdl(new_module_state_fid, &modules->items[i]);
        fclose(new_module_state_fid);
        if (ret == EXIT_FAILURE) goto exit_cleanup; 
    }

    exit_cleanup:
        if (old_config_fid != nullptr) {    
            free_config(old_config);
        } 
        free_config(new_config);
    return ret;
}
