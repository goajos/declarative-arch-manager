#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kdl/kdl.h>
#include <glob.h>

// will be added in glibc 2.43
void free_sized(void *ptr, size_t /*size*/)
{
    free(ptr);
}

#define da_append(xs, x)\
    do {\
        if (xs.count >= xs.capacity) {\
            if (xs.capacity == 0) xs.capacity = 256;\
            else xs.capacity *= 2;\
            xs.items = reallocarray(xs.items, xs.capacity, sizeof(*xs.items));\
        }\
        xs.items[xs.count++] = x;\
    } while(0)

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


static size_t read_func(void *user_data, char *buf, size_t bufsize)
{
    FILE *fid = (FILE *)user_data;
    return fread(buf, 1, bufsize, fid);
}

char *copy_data(const char *str)
{
    size_t str_size = strlen(str) + 1;
    char *dest = (char *)malloc(str_size);
    if (dest) return strcpy(dest, str);
    else return nullptr;
}

void parse_config_kdl(FILE *fid, Hosts *hosts)
{
   kdl_parser *parser = kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS); 
   
   bool eof = false;
   int node_depth = 0;
   const char *node_data = nullptr;
   
   while(1) {
        kdl_event_data *parsed_event = kdl_parser_next_event(parser);
        kdl_event event = parsed_event->event;
        const char *name_data = parsed_event->name.data;
        kdl_type value_type = parsed_event->value.type;
        bool boolean = parsed_event->value.boolean;
    
        while(1) {
            if (event == KDL_EVENT_START_NODE) {
                node_depth += 1;
                printf("Node depth inc: %d\n", node_depth);
                if (name_data) printf("Start node: %s\n", name_data);
                if (node_depth == 1) {
                    // TODO: set up a goto error block
                    if (strcmp(name_data, "config") != 0) printf("goto Error: Not a valid config...\n"); 
                } else if (node_depth == 2) {
                    node_data = name_data;
                }
                break;
            } else if (event == KDL_EVENT_END_NODE) {
                node_depth -= 1;
                printf("Node depth dec: %d\n", node_depth);
                printf("End node\n");
                break;
            } else if (event == KDL_EVENT_ARGUMENT) {
                if (value_type == KDL_TYPE_BOOLEAN) {
                    char *dest = copy_data(node_data);
                    if (dest) {
                        Host host = { dest, boolean };
                        da_append((*hosts), host);
                    }
                    printf("Event argument node: %b\n", boolean);
                }
                break;
            } else if (event == KDL_EVENT_EOF) {
                eof = true;
                break;
            }
        }

        if (eof) break; // outer while break
   }

   kdl_destroy_parser(parser); // parser cleans up
}

void parse_host_kdl(FILE *fid, Modules *modules, Services *services, Hooks *hooks)
{
   kdl_parser *parser = kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS); 
   
   bool eof = false;
   int node_depth = 0;
   char *node_data_d2 = nullptr;
   const char *node_data_d3 = nullptr;
   bool user_type_flag;
   
   while(1) {
        kdl_event_data *parsed_event = kdl_parser_next_event(parser);
        kdl_event event = parsed_event->event;
        const char *name_data = parsed_event->name.data;
        kdl_type value_type = parsed_event->value.type;
        bool boolean = parsed_event->value.boolean;
    
        while(1) {
            if (event == KDL_EVENT_START_NODE) {
                node_depth += 1;
                printf("Node depth inc: %d\n", node_depth);
                if (name_data) printf("Start node: %s\n", name_data);
                if (node_depth == 1) {
                    // TODO: set up a go to error block
                    if (strcmp(name_data, "host") != 0) printf("goto Error: Not a valid host...\n"); 
                    else user_type_flag = false; // false for system
                } else if (node_depth == 2) {
                    // manual clean up before copying new data
                    node_data_d2 = copy_data(name_data);
                } else if (node_depth == 3) {
                    node_data_d3 = name_data;
                }
                break;
            } else if (event == KDL_EVENT_END_NODE) {
                node_depth -= 1;
                printf("Node depth dec: %d\n", node_depth);
                printf("End node\n");
                break;
            } else if (event == KDL_EVENT_ARGUMENT) {
                if (value_type == KDL_TYPE_BOOLEAN) {
                    char *dest = copy_data(node_data_d3);
                    if (dest) {
                        if (strcmp(node_data_d2, "modules") == 0) {
                            Module module = { dest, boolean };
                            da_append((*modules), module);
                        } else if (strcmp(node_data_d2, "services") == 0) {
                            Service service = { dest, boolean, user_type_flag };
                            da_append((*services), service);
                        } else if (strcmp(node_data_d2, "hooks") == 0) {
                            Hook hook = { dest, boolean, user_type_flag };
                            da_append((*hooks), hook);
                        }
                    }
                    printf("Event argument node: %b\n", boolean);
                }
                break;
            } else if (event == KDL_EVENT_EOF) {
                eof = true;
                break;
            }
        }

        if (eof) break; // outer while break
   }

   if (node_data_d2 != nullptr) {
        free(node_data_d2); // manual clean up for copied data
        node_data_d2 = nullptr;
    }
   kdl_destroy_parser(parser); // parser cleans the rest
}

void parse_module_kdl(FILE *fid, Packages *packages, Services *services, Hooks *hooks)
{
   kdl_parser *parser = kdl_create_stream_parser(&read_func, (void *)fid, KDL_DEFAULTS); 
   
   bool eof = false;
   int node_depth = 0;
   char *node_data_d2 = nullptr;
   const char *node_data_d3 = nullptr;
   bool user_type_flag;
   
   while(1) {
        kdl_event_data *parsed_event = kdl_parser_next_event(parser);
        kdl_event event = parsed_event->event;
        const char *name_data = parsed_event->name.data;
        kdl_type value_type = parsed_event->value.type;
        bool boolean = parsed_event->value.boolean;
    
        while(1) {
            if (event == KDL_EVENT_START_NODE) {
                node_depth += 1;
                printf("Node depth inc: %d\n", node_depth);
                if (name_data) printf("Start node: %s\n", name_data);
                if (node_depth == 1) {
                    // TODO: set up a go to error block
                    if (strcmp(name_data, "module") != 0) printf("goto Error: Not a valid module...\n"); 
                    else user_type_flag = true; // true for user 
                } else if (node_depth == 2) {
                    // manual clean up before copying new data
                    node_data_d2 = copy_data(name_data);
                } else if (node_depth == 3) {
                    node_data_d3 = name_data;
                }
                break;
            } else if (event == KDL_EVENT_END_NODE) {
                node_depth -= 1;
                printf("Node depth dec: %d\n", node_depth);
                printf("End node\n");
                break;
            } else if (event == KDL_EVENT_ARGUMENT) {
                if (value_type == KDL_TYPE_BOOLEAN) {
                    char *dest = copy_data(node_data_d3);
                    if (dest) {
                        if (strcmp(node_data_d2, "packages") == 0) {
                            Package package = { dest, boolean };
                            da_append((*packages), package);
                        } else if (strcmp(node_data_d2, "services") == 0) {
                            Service service = { dest, boolean, user_type_flag };
                            da_append((*services), service);
                        } else if (strcmp(node_data_d2, "hooks") == 0) {
                            Hook hook = { dest, boolean, user_type_flag };
                            da_append((*hooks), hook);
                        }
                    }
                    printf("Event argument node: %b\n", boolean);
                }
                break;
            } else if (event == KDL_EVENT_EOF) {
                eof = true;
                break;
            }
        }

        if (eof) break; // outer while break
   }

   if (node_data_d2 != nullptr) {
        free(node_data_d2); // manual clean up for copied data
        node_data_d2 = nullptr;
    }
   kdl_destroy_parser(parser); // parser cleans the rest
}

// clang main.c -o main -g -lc -lkdl -lm -std=c23 -Wall -Wextra -Werror -pedantic
// :lua require'dap'.repl.open()
int main()
{
    printf("Hello world!\n");

    Hosts hosts = {0};

    FILE *config_fid = fopen("config.kdl", "r");
    parse_config_kdl(config_fid, &hosts);
    fclose(config_fid);

    printf("Hosts count: %d\n", (int)hosts.count);
    char *active_host = nullptr;
    for (int i = 0; i < (int)hosts.count; ++i) {
        if (hosts.items[i].active) {
           active_host = hosts.items[i].item; 
           break; // only 1 active host assumption
        }
    }
    printf("active host: %s\n", active_host);

    glob_t host_globbuf;
    host_globbuf.gl_offs = 0; // not prepending any flags to glob
    glob("hosts/*.kdl", GLOB_DOOFFS, NULL, &host_globbuf);
    char *host_path = nullptr;
    for (size_t i = 0; i < host_globbuf.gl_pathc; ++i) {
        char *glob_path = host_globbuf.gl_pathv[i];
        if (strstr(glob_path, active_host) != nullptr) {
            host_path = copy_data(glob_path);
            break; // only 1 active host assumption
        }
    }
    
    Modules modules = {0};
    Services services = {0};
    Hooks hooks = {0};
    // if an active host is found
    if (host_path != nullptr) {
        char fidbuf[0x256];
        printf("%s\n", host_path);
        snprintf(fidbuf, sizeof(fidbuf), "%s", host_path);
        FILE *host_fid = fopen(fidbuf, "r");  
        parse_host_kdl(host_fid, &modules, &services, &hooks);
        fclose(host_fid);
        
        printf("Modules count: %d\n", (int)modules.count);
        for (int i = 0; i < (int)modules.count; ++i) {
            printf("Module %s - is active: %b\n", modules.items[i].item, modules.items[i].active);
        }
    }
    // no longer need hosts and host glob
    for (int i = 0; i < (int)hosts.count; ++i) {
        char *item = hosts.items[i].item;
        free_sized(item, sizeof(*item));
    }
    free_sized(hosts.items, hosts.capacity*sizeof(*hosts.items));
    hosts.items = nullptr;
    free_sized(host_path, sizeof(*host_path));
    globfree(&host_globbuf);

    Packages packages = {0};
    for (int i = 0; i < (int)modules.count; ++i) {
        Module module = modules.items[i];
        if (module.active) {
            char fidbuf[0x256];
            snprintf(fidbuf, sizeof(fidbuf), "modules/%s.kdl", module.item);
            printf("%s\n", fidbuf);
            FILE *module_fid = fopen(fidbuf, "r");
            parse_module_kdl(module_fid, &packages, &services, &hooks);
            fclose(module_fid);
        }

        printf("Packages count for module %s: %d\n", module.item, (int)packages.count);
        for (int i = 0; i < (int)packages.count; ++i) {
            printf("Package %s - is active: %b\n", packages.items[i].item, packages.items[i].active);
        }
    }

    printf("Service count: %d\n", (int)services.count);
    for (int i = 0; i < (int)services.count; ++i) {
        Service service = services.items[i];
        printf("Service %s - is active: %b and user service: %b\n", service.item, service.active, service.user_type);
    }


    for (int i = 0; i < (int)modules.count; ++i) {
        char *item = modules.items[i].item;
        free_sized(item, sizeof(*item));
    }
    free_sized(modules.items, modules.capacity*sizeof(*modules.items));
    modules.items = nullptr;

    for (int i = 0; i < (int)packages.count; ++i) {
        char *item = packages.items[i].item;
        free_sized(item, sizeof(*item));
    }
    free_sized(packages.items, packages.capacity*sizeof(*packages.items));
    packages.items = nullptr;
    
    for (int i = 0; i < (int)services.count; ++i) {
        char *item = services.items[i].item;
        free_sized(item, sizeof(*item));
    }
    free_sized(services.items, services.capacity*sizeof(*services.items));
    services.items = nullptr;

    for (int i = 0; i < (int)hooks.count; ++i) {
        char *item = hooks.items[i].item;
        free_sized(item, sizeof(*item));
    }
    free_sized(hooks.items, hooks.capacity*sizeof(*hooks.items));
    hooks.items = nullptr;

    return 0;
}
