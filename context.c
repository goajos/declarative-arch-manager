#include <glob.h>
#include "context.h"
#include "utils/parse.c"
#include "utils/debug.c"

constexpr size_t PATH_MAX = 4096;

//TODO: the context should always validate the current context/state before continuing
// do not put this logic in the parser, only parsing there
Context get_context(Context context)
{
    FILE *config_fid = fopen("/home/jappe/.config/damngr/config.kdl", "r");
    context = parse_config_kdl(config_fid, context);
    fclose(config_fid);

    String active_host = { .data = nullptr };
    for (int i = 0; i < (int)context.hosts.count; ++i) {
        if (context.hosts.items[i].active) {
           active_host = String(context.hosts.items[i].item.data);
           break; // only 1 active host assumption
        }
    }
    // TODO: no active host exit/go to error
    // if (active_host.data == nullptr) return 1; 
    printf("active host: %s\n", active_host.data);

    glob_t host_globbuf;
    host_globbuf.gl_offs = 0; // not prepending any flags to glob
    glob("/home/jappe/.config/damngr/hosts/*.kdl", GLOB_DOOFFS, NULL, &host_globbuf);
    String host_path = { .data = nullptr };
    for (size_t i = 0; i < host_globbuf.gl_pathc; ++i) {
        String glob_path = String(host_globbuf.gl_pathv[i]);
        if (string_contains(glob_path, active_host)) {
            host_path = string_copy(glob_path);
            break; // only 1 active host assumption
        }
    }
    // if an active host is found
    if (host_path.data != nullptr) {
        char fidbuf[PATH_MAX];
        printf("%s\n", host_path.data);
        snprintf(fidbuf, sizeof(fidbuf), "%s", host_path.data);
        FILE *host_fid = fopen(fidbuf, "r");  
        context = parse_host_kdl(host_fid, context);
        fclose(host_fid);

        debug_active(context.modules)
    }
    // TODO: no active host path exit/go to error
    // } else return 1;

    // no longer need host path and host glob
    free_sized(host_path.data, host_path.len);
    globfree(&host_globbuf);

    for (int i = 0; i < (int)context.modules.count; ++i) {
        Module module = context.modules.items[i];
        if (module.active) {
            char fidbuf[PATH_MAX]; 
            snprintf(fidbuf, sizeof(fidbuf), "/home/jappe/.config/damngr/modules/%s.kdl", module.item.data);
            printf("%s\n", fidbuf);
            FILE *module_fid = fopen(fidbuf, "r");
            context = parse_module_kdl(module_fid, context);
            fclose(module_fid);
        }

    }
    debug_active(context.packages);
    debug_active(context.aur_packages);
    debug_active_user_type(context.services);
    debug_active_user_type(context.hooks);
    
    return context;
}

void free_context(Context context)
{
    memory_cleanup(context.hosts);
    memory_cleanup(context.modules);
    memory_cleanup(context.packages);
   if (context.aur_helper.data != nullptr) {
        free_sized(context.aur_helper.data, context.aur_helper.len);
        context.aur_helper.data = nullptr;
    }
    memory_cleanup(context.aur_packages);
    memory_cleanup(context.services);
    memory_cleanup(context.hooks);
}
