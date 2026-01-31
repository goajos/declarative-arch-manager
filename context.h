#ifndef CONTEXT_H
#define CONTEXT_H
#include "utils/string.h"

constexpr size_t path_max = 4096;

typedef struct {
    String item;
    bool active;
} Host;

typedef struct {
    Host *items;
    size_t count;
    size_t capacity;
} Hosts;

typedef struct {
    String item;
    bool active;
} Module;

typedef struct {
    Module *items;
    size_t count;
    size_t capacity;
} Modules;

typedef struct {
    String item;
    bool active;
} Package;

typedef struct {
    Package *items;
    size_t count;
    size_t capacity;
} Packages;

typedef struct {
    String item;
    bool active;
    bool user_type;
} Service;

typedef struct {
    Service *items;
    size_t count;
    size_t capacity;
} Services;

typedef struct {
    String item;
    bool active;
    bool user_type;
} Hook;

typedef struct {
    Hook *items;
    size_t count;
    size_t capacity;
} Hooks;

typedef struct {
    Hosts hosts;
    Modules modules;
    Packages packages;
    Packages installed_packages;
    Packages aur_packages;
    Packages installed_aur_packages;
    Services services;
    Hooks hooks;
    String aur_helper;
} Context;

extern void init_context();

extern Context get_context(Context);

extern Context get_installed_packages(Context);

extern void free_context(Context);

#endif /* CONTEXT_H */
