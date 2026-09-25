#ifndef STATE_H
#define STATE_H
#include <stdio.h>

constexpr size_t path_max = 4096;

#define DYNAMIC_ARRAY_APPEND(da, item)\
    do {\
        if (da.count >= da.capacity) {\
            if (da.capacity == 0) da.capacity = 10;\
            else da.capacity *= 2;\
            da.items = realloc(da.items, da.capacity*sizeof(*da.items));\
        }\
        da.items[da.count++] = item;\
    } while(0)

struct permission {
    char* name;
    bool root;
};

struct permissions {
    struct permission* items;
    size_t capacity;
    size_t count;
};

struct packages {
    char** items;
    size_t capacity;
    size_t count;
};

struct module {
    char* name;
    bool sync;
    struct packages packages;
    struct packages aur_packages;
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

#endif /* STATE_H */
