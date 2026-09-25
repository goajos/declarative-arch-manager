#include <stdlib.h>
#include <string.h>
#include "state.h"

static int get_service_actions(struct service_actions* actions, struct permissions services) {
    for (size_t i = 0; i < services.count; ++i) {
        struct permission service = services.items[i];
        struct service_action action = { .name=service.name, .to_enable=true };
        DYNAMIC_ARRAY_APPEND((*actions), action);
    }
    return EXIT_SUCCESS;
}

static int get_package_actions(struct package_actions* actions, struct dynamic_array packages) {
    for (size_t i = 0; i < packages.count; ++i) {
        char* package = packages.items[i];
        struct package_action action = { .name=package, .to_install=true };
        DYNAMIC_ARRAY_APPEND((*actions), action);
    }
    return EXIT_SUCCESS;
}

static int get_dotfile_actions(struct dynamic_array* actions, bool sync, char* name) {
    if (sync) {
        // if sync is true add the module name for reference
        DYNAMIC_ARRAY_APPEND((*actions), name);
    }
    return EXIT_SUCCESS;
}

static int get_hook_actions(struct dynamic_array* root_actions,
                            struct dynamic_array* user_actions,
                            struct permissions hooks) {
    for (size_t i = 0; i < hooks.count; ++i) {
        struct permission hook = hooks.items[i];
        if (hook.root) {
            // if root is true add the module name to the root_actions for reference
            DYNAMIC_ARRAY_APPEND((*root_actions), hook.name);
        } else DYNAMIC_ARRAY_APPEND((* user_actions), hook.name); // otherwise to the user_actions
    }
    return EXIT_SUCCESS;
}

// static int determine_actions_from_services_diff(struct service_actions* actions,
//                                     struct permissions old_services,
//                                     struct permissions new_services) {
//     return EXIT_SUCCESS;
// }

static int determine_actions_from_modules_diff(
                                            [[maybe_unused]] struct dynamic_array* dotfile_actions,
                                            [[maybe_unused]] struct dynamic_array* root_hook_actions,
                                            [[maybe_unused]] struct dynamic_array* user_hook_actions,
                                            [[maybe_unused]] struct package_actions* package_actions,
                                            [[maybe_unused]] struct package_actions* aur_package_actions,
                                            [[maybe_unused]] struct service_actions* user_service_actions,
                                            struct modules old_modules,
                                            struct modules new_modules) {
    // TODO: do we want to have the modules sorted? deterministic?
    int ret = EXIT_SUCCESS; // TODO: remove this
    struct dynamic_array modules_to_install = { };
    struct dynamic_array modules_to_remove = { };
    struct dynamic_array modules_to_keep = { };
    COMPUTE_DYNAMIC_ARRAY_NAME_DIFF(&modules_to_install,
                                    &modules_to_remove,
                                    &modules_to_keep,
                                    old_modules,
                                    new_modules);
    // TODO: continue the work here... (what to do with the modules diff)
    return ret;
}

int determine_actions(struct config* old_config,
                struct config* new_config,
                struct service_actions* root_service_actions,
                struct dynamic_array* root_hook_actions,
                struct package_actions* package_actions,
                struct package_actions* aur_package_actions,
                struct dynamic_array* dotfile_actions,
                struct service_actions* user_service_actions,
                struct dynamic_array* user_hook_actions) {
    int ret;
    struct host old_host;
    struct host new_host;
    if (old_config == nullptr) {
        // no config state available
        no_config_state:
            new_host = new_config->active_host;
            ret = get_service_actions(root_service_actions, new_host.services);
            for (size_t i = 0; i < new_host.modules.count; ++i) {
                struct module module = new_host.modules.items[i];
                ret = get_dotfile_actions(dotfile_actions, module.sync, module.name);
                ret = get_hook_actions(root_hook_actions, user_hook_actions, module.hooks);
                ret = get_package_actions(package_actions, module.packages);
                ret = get_package_actions(aur_package_actions, module.aur_packages);
                ret = get_service_actions(user_service_actions, module.services);
            }
    } else {
        // config state available
        old_host = old_config->active_host;
        new_host = new_config->active_host;
        if (memcmp(old_host.name, new_host.name, strlen(old_host.name)) == 0) {
            // old and new config host are equal
            // check if there are changes to the host root services
            // TODO: uncomment this
            // ret = determine_actions_from_services_diff(root_service_actions,
            //                                             old_host.services,
            //                                             new_host.services); 
            ret = determine_actions_from_modules_diff(dotfile_actions,
                                                    root_hook_actions,
                                                    user_hook_actions,
                                                    package_actions,
                                                    aur_package_actions,
                                                    user_service_actions,
                                                    old_host.modules,
                                                    new_host.modules);
        } else if (memcmp(old_host.name, new_host.name, strlen(old_host.name)) != 0) {
            // old and new config host are not equal 
            // TODO: user confirmation that old state will be overwritten with new active host?
            goto no_config_state;
        }
    } 

    return ret;
}

