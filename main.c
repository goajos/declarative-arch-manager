#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include "kdl_parsing.c"

// clang main.c -o main -g -lc -lkdl -lm -std=c23 -Wall -Wextra -Werror -pedantic
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
    memory_cleanup(hosts);
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

    memory_cleanup(modules);
    memory_cleanup(packages);
    memory_cleanup(services);
    memory_cleanup(hooks);

    return 0;
}
