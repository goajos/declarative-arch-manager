#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kdl/kdl.h>

#define da_append(xs, x)\
    do {\
        if (xs.count >= xs.capacity) {\
            if (xs.capacity == 0) xs.capacity = 256;\
            else xs.capacity *= 2;\
            xs.items = realloc(xs.items, xs.capacity*sizeof(*xs.items));\
        }\
        xs.items[xs.count++] = x;\
    } while(0)


typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} Modules;

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} Packages;

typedef struct {
    char *item;
    bool type; 
} Service;

typedef struct {
    Service *items;
    size_t count;
    size_t capacity;
} Services;

typedef struct {
    char *item;
    bool type;
} Hook;

typedef struct {
    Hook *items;
    size_t count;
    size_t capacity;
} Hooks;


static size_t read_func(void* userdata, char* buf, size_t bufsize)
{
    FILE* fid = (FILE*)userdata;
    return fread(buf, 1, bufsize, fid);
}

char* copy_data(const char* str)
{
    size_t str_size = strlen(str) + 1;
    char *dest = (char *)malloc(str_size);
    if (dest) return strcpy(dest, str);
    else return NULL;
}

void parse_kdl_fid(FILE* fid, Modules* ms, Packages* ps, Services* ss, Hooks* hs)
{
    kdl_parser* parser = kdl_create_stream_parser(&read_func, (void*)fid, KDL_DEFAULTS);

    bool eof = false;
    int node_depth = 0;
    char *current_node_type = NULL;
    // 0 for system, 1 for user
    bool type_flag;
    while (1) {
        kdl_event_data *event = kdl_parser_next_event(parser);
        const char *event_name = NULL;
        kdl_event kevent = event->event;
        const char *data = event->name.data;
        kdl_type type = event->value.type;
        const kdl_str *kstring = NULL;
        kdl_number knum = event->value.number;
        kdl_number_type ntype = knum.type;

        switch(kevent) {
            case KDL_EVENT_EOF:
                event_name = "KDL_EVENT_EOF";
                // printf("%s\n", event_name);
                eof = true;
                break;
            case KDL_EVENT_START_NODE:
                event_name = "KDL_EVENT_START_NODE";
                printf("%s\n", event_name);
                node_depth += 1;
                printf("node depth incremented: %d\n", node_depth);
                if (data) {
                    printf("%s\n", data);
                    if (type_flag) printf("%b\n", type_flag);
                    if (node_depth == 1) {
                        if (strcmp(data, "host") == 0) type_flag = false; 
                        else if (strcmp(data, "module") == 0) type_flag = true; 
                    } else if (node_depth == 2) {
                        if (strcmp(data, "modules") == 0) current_node_type="modules";
                        else if (strcmp(data, "packages") == 0) current_node_type="packages";
                        else if (strcmp(data, "services") == 0) current_node_type="services";
                        else if (strcmp(data, "hooks") == 0) current_node_type="hooks";
                    } else if (node_depth == 3) {
                        if (strcmp(current_node_type, "modules") == 0) {
                            char* dest = copy_data(data);
                            if (dest) da_append((*ms), dest);
                        } else if (strcmp(current_node_type, "packages") == 0) {
                            char* dest = copy_data(data);
                            if (dest) da_append((*ps), dest);
                        } else if (strcmp(current_node_type, "services") == 0) {
                            char* dest = copy_data(data);
                            Service s = { dest, type_flag };
                            da_append((*ss), s);
                        } else if (strcmp(current_node_type, "hooks") == 0) {
                            char* dest = copy_data(data);
                            Hook h = { dest, type_flag };
                            da_append((*hs), h);
                        }
                    }
                }
                break;
            case KDL_EVENT_END_NODE:
                event_name = "KDL_EVENT_END_NODE";
                printf("%s\n", event_name);
                node_depth -= 1;
                printf("node depth decremented: %d\n", node_depth);
                if (data) {
                    printf("%s\n", data);
                }
                break;
            case KDL_EVENT_ARGUMENT:
                event_name = "KDL_EVENT_ARGUMENT";
                printf("%s\n", event_name);
                if (data) printf("%s\n", data);
                if (type == KDL_TYPE_STRING) {
                    kstring = &event->value.string;
                    kdl_owned_string kostring = kdl_clone_str(kstring); 
                    printf("%s\n", kostring.data);
                    kdl_free_string(&kostring);
                } else if (type == KDL_TYPE_NUMBER) {
                    switch(ntype) {
                        case KDL_NUMBER_TYPE_INTEGER:
                            if (knum.integer) printf("%lld\n", knum.integer);
                        case KDL_NUMBER_TYPE_FLOATING_POINT:
                            if (knum.floating_point) printf("%f\n", knum.floating_point);
                        case KDL_NUMBER_TYPE_STRING_ENCODED:
                            kstring = &event->value.number.string;
                            kdl_owned_string kostring = kdl_clone_str(kstring);
                            if (kostring.len != 0) {
                                printf("%s\n", kostring.data);
                            }
                            kdl_free_string(&kostring);
                    }
                }
                break;
            case KDL_EVENT_COMMENT:
                event_name = "KDL_EVENT_COMMENT";
                // printf("%s\n", event_name);
                break;
            case KDL_EVENT_PROPERTY:
                event_name = "KDL_EVENT_PROPERTY";
                // printf("%s\n", event_name);
                break;
            case KDL_EVENT_PARSE_ERROR:
                event_name = "KDL_EVENT_PARSE_ERROR";
                // printf("%s\n", event_name);
                break;
        }

        if (eof) break;
    }

    fclose(fid);
    kdl_destroy_parser(parser);
}

// install src/utils/ckdl-cat ~/.local/bin
// gcc main.c -o main -lkdl -lm -Wall -Wpedantic -Werror
int main()
{
    printf("Hello world!\n");
    Modules  ms = {0}; 
    Packages ps = {0};
    Services ss = {0};
    Hooks    hs = {0};
    
    FILE* host_fid = fopen("arch.kdl", "r");
    parse_kdl_fid(host_fid, &ms, &ps, &ss, &hs);

    char fidbuf[0x128];
    for (int i = 0; i < ms.count; ++i) {
        snprintf(fidbuf, sizeof(fidbuf), "modules/%s.kdl", ms.items[i]);
        FILE *module_fid = fopen(fidbuf, "r");  
        parse_kdl_fid(module_fid, &ms, &ps, &ss, &hs);
    }

    
    printf("ms count: %ld\n", ms.count);
    printf("ps count: %ld\n", ps.count);
    printf("ss count: %ld\n", ss.count);
    printf("hs count: %ld\n", hs.count);

    for (int i = 0; i < ms.count; ++i) printf("printing ms: %s\n", ms.items[i]);
    for (int i = 0; i < ps.count; ++i) printf("printing ps: %s\n", ps.items[i]);
    for (int i = 0; i < ss.count; ++i) printf("printing ss: %s - %b\n", ss.items[i].item, ss.items[i].type);
    for (int i = 0; i < hs.count; ++i) printf("printing hs: %s - %b\n", hs.items[i].item, hs.items[i].type);

    return 0;
}
