// TODO: add exit_failure to the context functions?
#include <dirent.h>
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
    int ret;
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

    ret = recursive_context_copy(st, src, fidbuf);

    return ret;
}

int get_aur_helper(char** aur_helper) {
    int ret;
    char fidbuf[path_max];
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr/config.kdl", get_user());
    FILE* config_fid = fopen(fidbuf, "r");
    ret = parse_config_kdl(config_fid, nullptr, aur_helper);
    fclose(config_fid);

    return ret;
}
