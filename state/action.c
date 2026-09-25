#include <stdlib.h>
#include <string.h>
#include "../commands/command_utils.h"
#include "state_utils.h"
#include "state.h"

static int get_service_actions(struct service_actions* actions,
                            struct dynamic_array services,
                            bool to_enable,
                            bool root) {
    for (size_t i = 0; i < services.count; ++i) {
        char* service = services.items[i];
        char* ret = string_copy(service);
        if (ret == nullptr) return EXIT_FAILURE;
        if (to_enable) {
            if (root) DYNAMIC_ARRAY_APPEND(actions->root_to_enable, ret);
            else DYNAMIC_ARRAY_APPEND(actions->user_to_enable, ret);
        } else {
            if (root) DYNAMIC_ARRAY_APPEND(actions->root_to_disable, ret);
            else DYNAMIC_ARRAY_APPEND(actions->user_to_disable, ret);
        }
    }
    return EXIT_SUCCESS;
}

static int get_package_actions(struct package_actions* actions,
                            struct dynamic_array packages,
                            bool to_install) {
    for (size_t i = 0; i < packages.count; ++i) {
        char* package = packages.items[i];
        char* ret = string_copy(package);
        if (ret == nullptr) return EXIT_FAILURE;
        if (to_install) DYNAMIC_ARRAY_APPEND(actions->to_install, ret);
        else DYNAMIC_ARRAY_APPEND(actions->to_remove, ret);
    }
    return EXIT_SUCCESS;
}

static int get_dotfile_actions(struct dotfile_actions* actions,
                            bool sync,
                            char* name,
                            bool to_sync) {
    if (sync) {
        char* ret;
        if (to_sync) {
            // to_sync=true, sync the module dotfiles
            ret = string_copy(name);
            if (ret == nullptr) return EXIT_FAILURE;
            else DYNAMIC_ARRAY_APPEND(actions->to_sync, ret);
        } else if (!to_sync) {
            // to_sync=false, unsync the module dotfiles
            ret = string_copy(name);
            if (ret == nullptr) return EXIT_FAILURE;
            else DYNAMIC_ARRAY_APPEND(actions->to_unsync, ret);
        }
    }
    return EXIT_SUCCESS;
}

static int get_hook_actions(struct hook_actions* actions,
                            struct dynamic_array hooks,
                            bool root) {
    for (size_t i = 0; i < hooks.count; ++i) {
        char* hook = hooks.items[i];
        char* ret = string_copy(hook);
        if (ret == nullptr) return EXIT_FAILURE;
        if (root) {
            // if root is true add the module name to the root_actions for reference
            DYNAMIC_ARRAY_APPEND(actions->root, ret);
        } else DYNAMIC_ARRAY_APPEND(actions->user, ret); // otherwise to the user_actions
    }
    return EXIT_SUCCESS;
}

static int get_module_actions(struct module module,
                            enum action action,
                            struct service_actions* service_actions,
                            struct package_actions* package_actions,
                            struct package_actions* aur_package_actions,
                            struct dotfile_actions* dotfile_actions,
                            struct hook_actions* hook_actions) {
    int ret;
    switch(action) {
        case TO_INSTALL:
            ret = get_service_actions(service_actions, module.user_services, true, false);
            if (ret == EXIT_FAILURE) return ret;
            ret = get_package_actions(package_actions, module.packages, true);
            if (ret == EXIT_FAILURE) return ret;
            ret = get_package_actions(aur_package_actions, module.aur_packages, true);
            if (ret == EXIT_FAILURE) return ret;
            ret = get_dotfile_actions(dotfile_actions, module.sync, module.name, true);
            if (ret == EXIT_FAILURE) return ret;
            ret = get_hook_actions(hook_actions, module.root_hooks, true);
            if (ret == EXIT_FAILURE) return ret;
            ret = get_hook_actions(hook_actions, module.user_hooks, false);
            if (ret == EXIT_FAILURE) return ret;
            break;
        case TO_REMOVE:
            ret = get_service_actions(service_actions, module.user_services, false, false);
            if (ret == EXIT_FAILURE) return ret;
            ret = get_package_actions(package_actions, module.packages, false);
            if (ret == EXIT_FAILURE) return ret;
            ret = get_package_actions(aur_package_actions, module.aur_packages, false);
            if (ret == EXIT_FAILURE) return ret;
            ret = get_dotfile_actions(dotfile_actions, module.sync, module.name, false);
            if (ret == EXIT_FAILURE) return ret;
            // can't reverse post install hooks...
            break;
    }
    return ret;
}


static int determine_actions_from_services_diff(struct dynamic_array old_services,
                                                struct dynamic_array new_services,
                                                struct service_actions* actions) {
    int ret;
    struct dynamic_array services_to_enable = { };
    struct dynamic_array services_to_disable = { };
    struct dynamic_array services_to_keep = { };
    COMPUTE_DYNAMIC_ARRAY_DIFF(&services_to_enable,
                            &services_to_disable,
                            &services_to_keep,
                            old_services,
                            new_services);
    for (size_t i = 0; i < services_to_enable.count; ++i) {
        // enable all "to enable" root services
        ret = get_service_actions(actions, services_to_enable, true, true);
        if (ret == EXIT_FAILURE) return ret;
    }
    for (size_t i = 0; i < services_to_disable.count; ++i) {
        // disable all "to disable" root services
        ret = get_service_actions(actions, services_to_disable, false, true);
        if (ret == EXIT_FAILURE) return ret;
    }
    // skip "to keep" root services (already enabled)

    return ret;
}

static int determine_actions_from_packages_diff(struct dynamic_array old_packages,
                                              struct dynamic_array new_packages,
                                              struct package_actions* package_actions) {
    int ret;
    struct dynamic_array packages_to_install = { };
    struct dynamic_array packages_to_remove = { };
    struct dynamic_array packages_to_keep = { };
    COMPUTE_DYNAMIC_ARRAY_DIFF(&packages_to_install,
                            &packages_to_remove,
                            &packages_to_keep,
                            old_packages,
                            new_packages);
    for (size_t i = 0; i < packages_to_install.count; ++i) {
        ret = get_package_actions(package_actions,
                                packages_to_install,
                                true);
        if (ret == EXIT_FAILURE) return ret;
    }
    for (size_t i = 0; i < packages_to_remove.count; ++i) {
        ret = get_package_actions(package_actions,
                                packages_to_remove,
                                false);
        if (ret == EXIT_FAILURE) return ret;
    }
    // packages_to_keep are ignored (already installed)

    return ret;
}

static int determine_actions_from_dotfiles_diff(struct module old_module,
                                              struct module new_module,
                                              struct dotfile_actions* dotfile_actions) {
    int ret;
    if (old_module.sync) {
        if (!new_module.sync) {
            // desync the dotfiles
            ret = get_dotfile_actions(dotfile_actions, new_module.sync, new_module.name, false);
            if (ret == EXIT_FAILURE) return ret;
        }
    } else if (!old_module.sync) {
        if (new_module.sync) {
            // sync the dotfiles
            ret = get_dotfile_actions(dotfile_actions, new_module.sync, new_module.name, true);
            if (ret == EXIT_FAILURE) return ret;
        }
    }
     
    return ret;    
}

// TODO: to implement a hook reset, delete the hook from the old hooks list!
static int determine_actions_from_hooks_diff(struct dynamic_array old_hooks,
                                            struct dynamic_array new_hooks,
                                            struct hook_actions* hook_actions,
                                            bool root) {
    int ret;
    struct dynamic_array hooks_to_install = { };
    struct dynamic_array hooks_to_remove = { };
    struct dynamic_array hooks_to_keep = { };
    COMPUTE_DYNAMIC_ARRAY_DIFF(&hooks_to_install,
                            &hooks_to_remove,
                            &hooks_to_keep,
                            old_hooks,
                            new_hooks);
    ret = get_hook_actions(hook_actions, hooks_to_install, root);
    // hooks_to_remove are ignored (can't undo a script)
    // hooks_to_keep are ignored (hooks have been ran in the past)

    return ret;
}

static int determine_actions_from_module_diff(struct module old_module,
                                            struct module new_module,
                                            struct service_actions* service_actions,
                                            struct package_actions* package_actions,
                                            struct package_actions* aur_package_actions,
                                            struct dotfile_actions* dotfile_actions,
                                            struct hook_actions* hook_actions) {
    int ret;
    ret = determine_actions_from_services_diff(old_module.user_services,
                                            new_module.user_services,
                                            service_actions);
    if (ret == EXIT_FAILURE) return ret;
    ret = determine_actions_from_packages_diff(old_module.packages,
                                            new_module.packages,
                                            package_actions);
    if (ret == EXIT_FAILURE) return ret;
    ret = determine_actions_from_packages_diff(old_module.aur_packages,
                                            new_module.aur_packages,
                                            aur_package_actions);
    if (ret == EXIT_FAILURE) return ret;
    ret = determine_actions_from_dotfiles_diff(old_module,
                                            new_module,
                                            dotfile_actions);
    if (ret == EXIT_FAILURE) return ret;
    ret = determine_actions_from_hooks_diff(old_module.root_hooks,
                                            new_module.root_hooks,
                                            hook_actions,
                                            true);
    if (ret == EXIT_FAILURE) return ret;
    ret = determine_actions_from_hooks_diff(old_module.user_hooks,
                                            new_module.user_hooks,
                                            hook_actions,
                                            false);

    return ret;
}

static int determine_actions_from_modules_diff(struct modules old_modules,
                                            struct modules new_modules,
                                            struct service_actions* service_actions,
                                            struct package_actions* package_actions,
                                            struct package_actions* aur_package_actions,
                                            struct dotfile_actions* dotfile_actions,
                                            struct hook_actions* hook_actions) {
    // TODO: do we want to have the modules sorted? deterministic?
    int ret;
    struct modules modules_to_install = { };
    struct modules modules_to_remove = { };
    struct modules modules_to_keep = { };
    COMPUTE_MODULES_DIFF(&modules_to_install,
                        &modules_to_remove,
                        &modules_to_keep,
                        old_modules,
                        new_modules);
    for (size_t i = 0; i < modules_to_install.count; ++i) { 
        ret = get_module_actions(modules_to_install.items[i],
                                TO_INSTALL,
                                service_actions,
                                package_actions,
                                aur_package_actions,
                                dotfile_actions,
                                hook_actions);
        if (ret == EXIT_FAILURE) return ret;
    }
    for (size_t i = 0; i < modules_to_remove.count; ++i) {
        ret = get_module_actions(modules_to_remove.items[i],
                                TO_REMOVE,
                                service_actions,
                                package_actions,
                                aur_package_actions,
                                dotfile_actions,
                                hook_actions);
        if (ret == EXIT_FAILURE) return ret;
    }
    // modules to keep gets filled with 2 modules instead (1 old and 1 new)
    for (size_t i = 0; i < modules_to_keep.count; i += 2) {
        ret = determine_actions_from_module_diff(modules_to_keep.items[i],
                                                modules_to_keep.items[i+1],
                                                service_actions,
                                                package_actions,
                                                aur_package_actions,
                                                dotfile_actions,
                                                hook_actions);
        if (ret == EXIT_FAILURE) return ret;
    }

    return ret;
}

int determine_actions(struct config* old_config,
                struct config* new_config,
                struct service_actions* service_actions,
                struct package_actions* package_actions,
                struct package_actions* aur_package_actions,
                struct dotfile_actions * dotfile_actions,
                struct hook_actions* hook_actions) {
    int ret;
    struct host old_host;
    struct host new_host;
    if (old_config->aur_helper == nullptr) {
        // no config state available
        no_config_state:
            new_host = new_config->active_host;
            ret = get_service_actions(service_actions, new_host.root_services, true, true);
            for (size_t i = 0; i < new_host.modules.count; ++i) {
                struct module module = new_host.modules.items[i];
                ret = get_service_actions(service_actions, module.user_services, true, false);
                if (ret == EXIT_FAILURE) return ret;
                ret = get_package_actions(package_actions, module.packages, true);
                if (ret == EXIT_FAILURE) return ret;
                ret = get_package_actions(aur_package_actions, module.aur_packages, true);
                if (ret == EXIT_FAILURE) return ret;
                ret = get_dotfile_actions(dotfile_actions, module.sync, module.name, true);
                if (ret == EXIT_FAILURE) return ret;
                ret = get_hook_actions(hook_actions, module.root_hooks, true);
                if (ret == EXIT_FAILURE) return ret;
                ret = get_hook_actions(hook_actions, module.user_hooks, false);
                if (ret == EXIT_FAILURE) return ret;
            }
    } else {
        // config state available
        old_host = old_config->active_host;
        new_host = new_config->active_host;
        if (memcmp(old_host.name, new_host.name, strlen(old_host.name)) == 0) {
            // old and new config host are equal
            ret = determine_actions_from_services_diff(old_host.root_services,
                                                    new_host.root_services, 
                                                    service_actions);
            if (ret == EXIT_FAILURE) return ret;
            ret = determine_actions_from_modules_diff(old_host.modules,
                                                    new_host.modules,
                                                    service_actions,
                                                    package_actions,
                                                    aur_package_actions,
                                                    dotfile_actions,
                                                    hook_actions);
            if (ret == EXIT_FAILURE) return ret;
        } else if (memcmp(old_host.name, new_host.name, strlen(old_host.name)) != 0) {
            // old and new config host are not equal 
            // TODO: user confirmation that old state will be overwritten with new active host?
            goto no_config_state;
        }
    } 

    return ret;
}

// TODO: how to make atomic
int handle_package_actions(struct package_actions package_actions) {
    int ret;
    if (package_actions.to_remove.count > 0) {
        ret = execute_package_remove_command(package_actions.to_remove);
    }
    if (package_actions.to_install.count > 0) {
        ret = execute_package_install_command(package_actions.to_install);
    }
    return ret;
}

// TODO: how to make atomic
int handle_aur_package_actions(struct package_actions aur_package_actions,
                            char* aur_helper) {
    int ret;
    if (aur_package_actions.to_remove.count > 0) {
        ret = execute_package_remove_command(aur_package_actions.to_remove);
    }
    if (aur_package_actions.to_install.count > 0) {
        char fidbuf[path_max];
        snprintf(fidbuf, sizeof(fidbuf), "/usr/bin/%s", aur_helper);
        ret = execute_aur_package_install_command(fidbuf,
                                            aur_helper,
                                            aur_package_actions.to_install);
    }
    return ret;
}

// TODO: how to make atomic?
int handle_service_actions(struct service_actions service_actions) {
    int ret;
    if (service_actions.root_to_disable.count > 0) {
        ret = execute_service_command(true, false, service_actions.root_to_disable);
    }
    if (service_actions.root_to_enable.count > 0) {
        ret = execute_service_command(true, true, service_actions.root_to_enable);
    }
    if (service_actions.user_to_disable.count > 0) {
        ret = execute_service_command(false, false, service_actions.user_to_disable);
    }
    if (service_actions.user_to_enable.count > 0) {
        ret = execute_service_command(false, true, service_actions.user_to_enable);
    }
    return ret;
}


// TODO: how to handle failure in the cleanup chain?
int free_actions(struct service_actions service_actions,
                struct package_actions package_actions,
                struct package_actions aur_package_actions,
                struct dotfile_actions dotfile_actions,
                struct hook_actions hook_actions) {
    DYNAMIC_ARRAY_FREE(service_actions.root_to_disable);
    DYNAMIC_ARRAY_FREE(service_actions.root_to_enable);
    DYNAMIC_ARRAY_FREE(service_actions.user_to_disable);
    DYNAMIC_ARRAY_FREE(service_actions.user_to_enable);
    DYNAMIC_ARRAY_FREE(package_actions.to_remove);
    DYNAMIC_ARRAY_FREE(package_actions.to_install);
    DYNAMIC_ARRAY_FREE(aur_package_actions.to_remove);
    DYNAMIC_ARRAY_FREE(aur_package_actions.to_install);
    DYNAMIC_ARRAY_FREE(dotfile_actions.to_unsync);
    DYNAMIC_ARRAY_FREE(dotfile_actions.to_sync);
    DYNAMIC_ARRAY_FREE(hook_actions.root);
    DYNAMIC_ARRAY_FREE(hook_actions.user);

    return EXIT_SUCCESS;
}
