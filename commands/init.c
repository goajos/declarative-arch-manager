#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "command_utils.h"

static int copy_file(char* src, char* dst)
{
    FILE* src_fid = fopen(src, "r");
    if (src_fid == nullptr) return EXIT_FAILURE;
    FILE* dst_fid = fopen(dst, "w");
    if (dst_fid == nullptr) return EXIT_FAILURE;
    char c;
    while ((c = fgetc(src_fid)) != EOF) {
        fputc(c, dst_fid);
    }
    fclose(src_fid);
    fclose(dst_fid);

    return EXIT_SUCCESS;
}

static int recursive_init_state(struct stat st, char* src, char* dst)
{
    int ret;
    stat(src, &st); // get src file stats
    if (S_ISDIR(st.st_mode)) { // src is a directory
        DIR* damngr_dir = opendir(src); 
        if (damngr_dir == nullptr) return EXIT_FAILURE;
        struct dirent* ent;
        while ((ent = readdir(damngr_dir)) != nullptr) {
            char* d_name = ent->d_name;
            // skip the . and .. dir entries
            if (memcmp(d_name, ".", 1) == 0 || memcmp(d_name, "..", 2) == 0) continue; // don't break here
            char src_fidbuf[path_max]; 
            char dst_fidbuf[path_max]; 
            snprintf(src_fidbuf, sizeof(src_fidbuf), "%s/%s", src, d_name);
            snprintf(dst_fidbuf, sizeof(dst_fidbuf), "%s/%s", dst, d_name);                 
            // stats returns -1 if the path doesn't exist
            if (stat(dst_fidbuf, &st) == -1) {
                // ensure the dst path is a directory
                if (strstr(dst_fidbuf, ".kdl") == nullptr) mkdir(dst_fidbuf, 0777);
            }
            ret = recursive_init_state(st, src_fidbuf, dst_fidbuf);
        }
        closedir(damngr_dir);
    } else if (S_ISREG(st.st_mode)) { // src is a file
        ret = copy_file(src, dst);
    }

    return ret;
}

int damngr_init() {
    puts("hello from damngr init...");
    int ret;
    struct stat st;
    char* src = "damngr";
    char fidbuf[path_max]; 
    // create the .local/share/damngr folder
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.local/share/damngr", get_user());
    // stat returns -1 if fidbuf doesn't exist
    if (stat(fidbuf, &st) == -1) mkdir(fidbuf, 0777);

    // create the .config/damngr folder
    snprintf(fidbuf, sizeof(fidbuf), "/home/%s/.config/damngr", get_user());
    // stat returns -1 if fidbuf doesn't exist
    if (stat(fidbuf, &st) == -1) mkdir(fidbuf, 0777);
    else {
        puts("Config already exists:");
        puts("Remove the .config/damngr folder before calling init again");
        ret = EXIT_FAILURE;
        goto exit_cleanup; 
    }
    ret = recursive_init_state(st, src, fidbuf);

    exit_cleanup:
    return ret;
}
