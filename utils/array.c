#include <stdlib.h>
#include "string.c"

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

#define array_append(xs, x)\
    do {\
        if (xs.count >= xs.capacity) {\
            if (xs.capacity == 0) xs.capacity = 256;\
            else xs.capacity *= 2;\
            xs.items = realloc(xs.items, xs.capacity*sizeof(*xs.items));\
        }\
        xs.items[xs.count++] = x;\
    } while(0)
