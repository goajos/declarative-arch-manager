#ifndef STATE_H
#define STATE_H
#include <stdio.h>

constexpr int path_max = 4096;

#define DYNAMIC_ARRAY_APPEND(da, item)\
    do {\
        if (da.count >= da.capacity) {\
            if (da.capacity == 0) da.capacity = 10;\
            else da.capacity *= 2;\
            da.items = realloc(da.items, da.capacity*sizeof(*da.items));\
        }\
        da.items[da.count++] = item;\
    } while(0)

#define DYNAMIC_ARRAY_FREE(da)\
    do {\
        for (size_t i = 0; i < da.count; ++i) {\
            char* item = da.items[i];\
            free_sized(item, strlen(item));\
            item = nullptr;\
        }\
        free_sized(da.items, da.capacity*sizeof(*da.items));\
        da.items = nullptr;\
    } while(0)

#define MODULES_FREE(modules)\
    do {\
        for (size_t i = 0; i < modules.count; ++i) {\
            char* item = modules.items[i].name;\
            free_sized(item, strlen(item));\
            item = nullptr;\
        }\
        free_sized(modules.items, modules.capacity*sizeof(*modules.items));\
        modules.items = nullptr;\
    } while(0)

struct dynamic_array {
    char** items;
    size_t capacity;
    size_t count;
};

struct module {
    char* name;
    bool link;
    struct dynamic_array user_services;
    struct dynamic_array packages;
    struct dynamic_array aur_packages;
    struct dynamic_array pre_root_hooks;
    struct dynamic_array post_root_hooks;
    struct dynamic_array pre_user_hooks;
    struct dynamic_array post_user_hooks;
};

struct modules {
    struct module* items;
    size_t capacity;
    size_t count;
};

struct host {
    char* name;
    struct modules modules;
    struct dynamic_array root_services;
};

struct config {
    char* aur_helper;
    struct host active_host;
};

int parse_config_kdl(FILE* fid, struct config* config);
int write_config_kdl(FILE* fid, struct config* config);
int parse_host_kdl(FILE* fid, struct host* host);
int write_host_kdl(FILE* fid, struct host* host);
int parse_module_kdl(FILE* fid, struct module* module);
int write_module_kdl(FILE* fid, struct module* module);

#define COMPUTE_DYNAMIC_ARRAY_DIFF(to_install, to_remove, to_keep, old_da, new_da)\
    do {\
        qsort(old_da.items, old_da.count, sizeof(old_da.items[0]), qcharcmp);\
        qsort(new_da.items, new_da.count, sizeof(new_da.items[0]), qcharcmp);\
        size_t i = 0;\
        size_t j = 0;\
        while (i < old_da.count && j < new_da.count) {\
            int res = strcmp(old_da.items[i], new_da.items[j]);\
            if (res < 0) {\
                DYNAMIC_ARRAY_APPEND((*to_remove), old_da.items[i]);\
                ++i;\
            } else if (res > 0) {\
                DYNAMIC_ARRAY_APPEND((*to_install), new_da.items[j]);\
                ++j;\
            } else {\
                DYNAMIC_ARRAY_APPEND((*to_keep), old_da.items[i]);\
                DYNAMIC_ARRAY_APPEND((*to_keep), new_da.items[i]);\
                ++i;\
                ++j;\
            }\
        }\
        while (i < old_da.count) {\
            DYNAMIC_ARRAY_APPEND((*to_remove), old_da.items[i]);\
            ++i;\
        }\
        while (j < new_da.count) {\
            DYNAMIC_ARRAY_APPEND((*to_install), new_da.items[j]);\
            ++j;\
        }\
    } while(0)

#define COMPUTE_MODULES_DIFF(to_install, to_remove, to_keep, old_modules, new_modules)\
    do {\
        qsort(old_modules.items, old_modules.count, sizeof(old_modules.items[0]), qnamecmp);\
        qsort(new_modules.items, new_modules.count, sizeof(new_modules.items[0]), qnamecmp);\
        size_t i = 0;\
        size_t j = 0;\
        while (i < old_modules.count && j < new_modules.count) {\
            int res = strcmp(old_modules.items[i].name, new_modules.items[j].name);\
            if (res < 0) {\
                DYNAMIC_ARRAY_APPEND((*to_remove), old_modules.items[i]);\
                ++i;\
            } else if (res > 0) {\
                DYNAMIC_ARRAY_APPEND((*to_install), new_modules.items[j]);\
                ++j;\
            } else {\
                DYNAMIC_ARRAY_APPEND((*to_keep), old_modules.items[i]);\
                DYNAMIC_ARRAY_APPEND((*to_keep), new_modules.items[i]);\
                ++i;\
                ++j;\
            }\
        }\
        while (i < old_modules.count) {\
            DYNAMIC_ARRAY_APPEND((*to_remove), old_modules.items[i]);\
            ++i;\
        }\
        while (j < new_modules.count) {\
            DYNAMIC_ARRAY_APPEND((*to_install), new_modules.items[j]);\
            ++j;\
        }\
    } while(0)
 
struct service_actions {
    struct dynamic_array root_to_disable;
    struct dynamic_array root_to_enable;
    struct dynamic_array user_to_disable;
    struct dynamic_array user_to_enable;
};

struct package_actions {
    struct dynamic_array to_remove;
    struct dynamic_array to_install;
};

struct dotfile_actions {
    struct dynamic_array to_unlink;
    struct dynamic_array to_link;
};

struct hook_actions {
    struct dynamic_array root;
    struct dynamic_array user;
};

enum action {
    TO_INSTALL,
    TO_REMOVE,
};

int determine_actions(struct config* old_config,
                        struct config* new_config,
                        struct service_actions* service_actions,
                        struct package_actions* package_actions,
                        struct package_actions* aur_package_actions,
                        struct dotfile_actions* dotfile_actions,
                        struct hook_actions* pre_hook_actions,
                        struct hook_actions* post_hook_actions);

int handle_package_actions(struct package_actions package_actions);
int handle_aur_package_actions(struct package_actions aur_package_actions, char* aur_helper);
int handle_service_actions(struct service_actions service_actions);
int handle_dotfile_actions(struct dotfile_actions dotfile_actions);
int handle_hook_actions(struct hook_actions hook_actions);

int free_config(struct config config);
int free_host(struct host host);
int free_module(struct module module);
int free_actions(struct service_actions service_actions,
                struct package_actions package_actions,
                struct package_actions aur_package_actions,
                struct dotfile_actions dotfile_actions,
                struct hook_actions pre_hook_actions,
                struct hook_actions post_hook_actions);

#endif /* STATE_H */
