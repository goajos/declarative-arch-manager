#include <stdlib.h>
#include <string.h>
#include "state.h"
#include "state_utils.h"

int calculate_config_diff(struct config* old_config,
                        struct config* new_config
                        // struct actions* module_actions,
                        // struct actions* package_actions,
                        // struct actions* service_actions,
                        // struct actions* hook_actions
                        ) {
    // int ret;
    int ret = EXIT_SUCCESS; //TODO: remove this
    struct host old_host = old_config->active_host;
    struct host new_host = new_config->active_host;
    if (memcmp(old_host.name, new_host.name, strlen(old_host.name)) != 0) return EXIT_FAILURE;
    struct dynamic_array old_modules = { };
    struct dynamic_array new_modules = { };
    for (size_t i = 0; i < old_host.modules.count; ++i) {
        struct module module = old_host.modules.items[i];
        DYNAMIC_ARRAY_APPEND(old_modules, module.name);
    }
    for (size_t i = 0; i < new_host.modules.count; ++i) {
        struct module module = new_host.modules.items[i];
        DYNAMIC_ARRAY_APPEND(new_modules, module.name);
    }

    (void)qstrcmp;
    // qsort(old_packages, old_packages->count, sizeof(char *), qstrcmp);
    // qsort(new_packages, new_packages->count, sizeof(char *), qstrcmp);
    // size_t i = 0;
    // size_t j = 0;
    // while (i < old_packages->count && j < new_packages->count) {
    //     char* p1 = old_packages->items[i];
    //     char* p2 = old_packages->items[j];
    //     int res = strcmp(p1, p2);
    //     if (res < 0) { // p1 < p2
    //         DYNAMIC_ARRAY_APPEND((*to_remove), p1); 
    //         ++i;
    //     } else if (res > 0) { // p1 > p2
    //         DYNAMIC_ARRAY_APPEND((*to_install), p2); 
    //         ++j;
    //     } else { // packages are equal
    //         DYNAMIC_ARRAY_APPEND((*to_keep), p1); 
    //         ++i;
    //         ++j;
    //     }
    // }
    // while (i < old_packages->count) {
    //     DYNAMIC_ARRAY_APPEND((*to_remove), old_packages->items[i]); 
    //     ++i;
    // }
    // while (j < new_packages->count) {
    //     DYNAMIC_ARRAY_APPEND((*to_install), old_packages->items[j]); 
    //     ++j;
    // }

    return ret;
}

