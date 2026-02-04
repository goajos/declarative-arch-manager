// TODO: add exit_failure to the context functions?
#include <dirent.h>
#include <glob.h>
#include <sys/stat.h>
#include "context.h"
#include "utils/kdl_parse.c"
#include "utils/user.c"

static int copy_context_file(char* src, char* dst)
{
    FILE *src_fid = fopen(src, "r");
    FILE *dst_fid = fopen(dst, "w");

    char c;
    while ((c = fgetc(src_fid)) != EOF) {
        fputc(c, dst_fid);
    }

    fclose(src_fid);
    fclose(dst_fid);

    return EXIT_SUCCESS;
}

static int recursive_context_copy(struct stat st, char* src, char* dst)
{
    int ret;
    stat(src, &st); // get src file stats

    if (S_ISDIR(st.st_mode)) { // src is a directory
        DIR *damngr_dir = opendir(src); 
        struct dirent *ent;
        while ((ent = readdir(damngr_dir)) != NULL) {
            char *d_name = ent->d_name;
            if (memcmp(d_name, ".", 1) == 0 || memcmp(d_name, "..", 2) == 0) continue; // don't break here
            char src_path[path_max]; 
            char dst_path[path_max]; 
            snprintf(src_path, sizeof(src_path), "%s/%s", src, d_name);
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, d_name);                 
            // stats returns -1 if the dst path doesn't exist
            if (stat(dst_path, &st) == -1) {
                // ensure the dst path is a directory
                if (strstr(dst_path, ".kdl") == NULL) mkdir(dst_path, 0777);
            }
            ret = recursive_context_copy(st, src_path, dst_path);
        }
        closedir(damngr_dir);
    } else if (S_ISREG(st.st_mode)) { // src is a file
        ret = copy_context_file(src, dst);
    }

    return ret;
}

int init_context() {
    struct stat st;
    char* src = "damngr";
    char fidbuf[path_max];
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr", get_user());
    // stat returns -1 if the fidbuf doesn't exist
    if (stat(fidbuf, &st) == -1) mkdir(fidbuf, 0777);
    else {
        puts("Config already exists:");
        puts("Remove the ~/.config/damngr folder before calling damngr init again");
        return EXIT_FAILURE;
    }

    int ret = recursive_context_copy(st, src, fidbuf);

    return ret;
}

static int get_config_context(struct dynamic_array* hosts, char** aur_helper) {
    char fidbuf[path_max];
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/config.kdl", get_user());
    FILE* config_fid = fopen(fidbuf, "r");
    int ret = parse_config_kdl(config_fid, hosts, aur_helper);
    fclose(config_fid);
    
    return ret;
}

int get_aur_helper(char** aur_helper) {
    int ret = get_config_context(nullptr, aur_helper);
    return ret;
}

// static int get_active_host(struct dynamic_array hosts, char** active_host) {
//     size_t active_idx = 0;
//     size_t cnt = 0;
//     for (size_t i = 0; i < hosts.count; ++i) {
//         if (hosts.elements[i].active) {
//             active_idx = i;
//             cnt += 1;
//         }
//     }
//
//     if (cnt != 1) return EXIT_FAILURE;
//     // only set the active host with a valid config
//     else {
//         *active_host = hosts.elements[active_idx].element;
//         return EXIT_SUCCESS;
//     }
// }

static int get_active_host_path(char** active_host, char** host_path) {
    glob_t globbuf;
    globbuf.gl_offs = 0; // not prepending any flags to glob
    char fidbuf[path_max];
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/hosts/*.kdl", get_user());
    glob(fidbuf, GLOB_DOOFFS, NULL, &globbuf);
    for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
        char* glob_path = globbuf.gl_pathv[i];
        if (strstr(glob_path, *active_host) != NULL) {
            *host_path = string_copy(glob_path);
            globfree(&globbuf);
            return EXIT_SUCCESS;
        }
    } 
    
    globfree(&globbuf);
    return EXIT_FAILURE;
}


static int get_host_context(struct active_dynamic_array* modules, struct active_dynamic_array* services, struct active_dynamic_array* hooks, char* host_path) {
    FILE* host_fid = fopen(host_path, "r");
    int ret = parse_host_kdl(host_fid, modules, services, hooks);
    fclose(host_fid);

    return ret;
}

static int get_module_context(struct active_dynamic_array* packages, struct active_dynamic_array* aur_packages, struct active_dynamic_array* services, struct active_dynamic_array* hooks, char* module_path) { 
    FILE* module_fid = fopen(module_path, "r");
    int ret = parse_module_kdl(module_fid, packages, aur_packages, services, hooks);
    fclose(module_fid);

    return ret;
}

int get_sync_context(char** aur_helper, struct dynamic_array* packages, struct dynamic_array* aur_packages, struct dynamic_array* root_services, struct dynamic_array* root_hooks, struct active_dynamic_array* user_services, struct active_dynamic_array* user_hooks) {
    int ret;
    struct active_dynamic_array hosts = { };

    ret = get_config_context(&hosts, aur_helper);
    if (ret == EXIT_FAILURE) goto exit_cleanup;

    char* active_host = nullptr;
    ret = get_active_host(hosts, &active_host);
    if (ret == EXIT_FAILURE) goto exit_cleanup;

    char* host_path = nullptr;
    ret = get_active_host_path(&active_host, &host_path);
    if (ret == EXIT_FAILURE) goto exit_cleanup;
    
    struct active_dynamic_array modules = { };
    ret = get_host_context(&modules, root_services, root_hooks, host_path);
    if (ret == EXIT_FAILURE) goto exit_cleanup;

    for (size_t i = 0; i < modules.count; ++i) {
        struct active_element module = modules.elements[i];
        if (module.active) {
            char fidbuf[path_max];
            snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/modules/%s.kdl", get_user(), module.element);
            ret = get_module_context(packages, aur_packages, user_services, user_hooks, fidbuf);
            if (ret == EXIT_FAILURE) goto exit_cleanup;
        }
    }

    exit_cleanup:
        free_sized(host_path, strlen(host_path));
        host_path = nullptr; 
        // active host is a pointer to a host so gets cleaned up here
        DYNAMIC_ARRAY_FREE(hosts);
        DYNAMIC_ARRAY_FREE(modules);

        return ret;
}
