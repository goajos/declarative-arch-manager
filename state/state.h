#ifndef STATE_H
#define STATE_H
#include <stdio.h>
#include "state_utils.h"

#define DYNAMIC_ARRAY_APPEND(da, item)\
    do {\
        if (da.count >= da.capacity) {\
            if (da.capacity == 0) da.capacity = 10;\
            else da.capacity *= 2;\
            da.items = realloc(da.items, da.capacity*sizeof(*da.items));\
        }\
        da.items[da.count++] = item;\
    } while(0)

#define DYNAMIC_ARRAY_NAME_FREE(da)\
    do {\
        for (size_t i = 0; i < da.count; ++i) {\
            char* item = da.items[i].name;\
            free_sized(item, strlen(item));\
            item = nullptr;\
        }\
        free_sized(da.items, da.capacity*sizeof(*da.items));\
        da.items = nullptr;\
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

struct permission {
    char* name;
    bool root;
};

// a dynamic array with a permission item instead of a char*
struct permissions {
    struct permission* items;
    size_t capacity;
    size_t count;
};

struct dynamic_array {
    char** items;
    size_t capacity;
    size_t count;
};

struct module {
    char* name;
    bool sync;
    struct dynamic_array packages;
    struct dynamic_array aur_packages;
    struct permissions services;
    struct permissions hooks;
};

struct modules {
    struct module* items;
    size_t capacity;
    size_t count;
};

struct host {
    char* name;
    struct modules modules;
    struct permissions services;
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

#define COMPUTE_DYNAMIC_ARRAY_NAME_DIFF(to_install, to_remove, to_keep, old_da, new_da)\
    do {\
        qsort(old_da.items, old_da.count, sizeof(old_da.items[0]), qnamecmp);\
        qsort(new_da.items, new_da.count, sizeof(new_da.items[0]), qnamecmp);\
        size_t i = 0;\
        size_t j = 0;\
        while (i < old_da.count && j < new_da.count) {\
            int res = strcmp(old_da.items[i].name, new_da.items[j].name);\
            if (res < 0) {\
                DYNAMIC_ARRAY_APPEND((*to_remove), old_da.items[i].name);\
                ++i;\
            } else if (res > 0) {\
                DYNAMIC_ARRAY_APPEND((*to_install), new_da.items[j].name);\
                ++j;\
            } else {\
                DYNAMIC_ARRAY_APPEND((*to_keep), old_da.items[i].name);\
                ++i;\
                ++j;\
            }\
        }\
        while (i < old_da.count) {\
            DYNAMIC_ARRAY_APPEND((*to_remove), old_da.items[i].name);\
            ++i;\
        }\
        while (j < new_da.count) {\
            DYNAMIC_ARRAY_APPEND((*to_install), new_da.items[j].name);\
            ++j;\
        }\
    } while(0)

struct service_action {
    char* name;
    bool to_enable;
};
struct service_actions {
    struct service_action* items;
    size_t capacity;
    size_t count;
};
struct package_action {
    char* name;
    bool to_install;
};
struct package_actions {
    struct package_action* items;
    size_t capacity;
    size_t count;
};

int determine_actions(struct config* old_config,
                        struct config* new_config,
                        struct service_actions* root_service_actions,
                        struct dynamic_array* root_hook_actions,
                        struct package_actions* package_actions,
                        struct package_actions* aur_package_actions,
                        struct dynamic_array* dotfile_actions,
                        struct service_actions* user_service_actions,
                        struct dynamic_array* user_hook_actions);

int free_config(struct config config);
int free_host(struct host host);
int free_module(struct module module);

#endif /* STATE_H */
