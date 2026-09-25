#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *item;
    bool active;
} Host;

typedef struct {
    Host *items;
    size_t count;
    size_t capacity;
} Hosts;

typedef struct {
    char *item;
    bool active;
} Module;

typedef struct {
    Module *items;
    size_t count;
    size_t capacity;
} Modules;

typedef struct {
    char *item;
    bool active;
} Package;

typedef struct {
    Package *items;
    size_t count;
    size_t capacity;
} Packages;

typedef struct {
    char *item;
    bool active;
    bool user_type;
} Service;

typedef struct {
    Service *items;
    size_t count;
    size_t capacity;
} Services;

typedef struct {
    char *item;
    bool active;
    bool user_type;
} Hook;

typedef struct {
    Hook *items;
    size_t count;
    size_t capacity;
} Hooks;

# define memory_cleanup(xs)\
    for (int i = 0; i < (int)xs.count; ++i) {\
        char *item = xs.items[i].item;\
        free_sized(item, sizeof(*item));\
    }\
    free_sized(xs.items, xs.capacity*sizeof(*modules.items));\
    xs.items = nullptr;


#define da_append(xs, x)\
    do {\
        if (xs.count >= xs.capacity) {\
            if (xs.capacity == 0) xs.capacity = 256;\
            else xs.capacity *= 2;\
            xs.items = realloc(xs.items, xs.capacity*sizeof(*xs.items));\
        }\
        xs.items[xs.count++] = x;\
    } while(0)

// will be added in glibc 2.43
void free_sized(void *ptr, size_t /*size*/)
{
    free(ptr);
}

char *copy_data(const char *str)
{
    size_t str_size = strlen(str) + 1;
    char *dest = (char *)malloc(str_size);
    if (dest) return strcpy(dest, str);
    else return nullptr;
}

static size_t read_func(void *user_data, char *buf, size_t bufsize)
{
    FILE *fid = (FILE *)user_data;
    return fread(buf, 1, bufsize, fid);
}
