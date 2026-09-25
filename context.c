#include <glob.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include "utils/parse.c"
#include "utils/debug.c"

void copy_context_file(String src, String dst)
{
    FILE *src_fid = fopen(src.data, "r");
    FILE *dst_fid = fopen(dst.data, "w");

    char c;
    while ((c = fgetc(src_fid)) != EOF) {
        fputc(c, dst_fid);
    }

    fclose(src_fid);
    fclose(dst_fid);
}

void recursive_context_copy(struct stat st, String src, String dst)
{
    stat(src.data, &st);

    if (S_ISDIR(st.st_mode)) {
        DIR *damngr_dir = opendir(src.data); 
        struct dirent *ent;
        while ((ent = readdir(damngr_dir)) != NULL) {
            char *d_name = ent->d_name;
            if (string_equal(String(d_name), String(".")) || string_equal(String(d_name), String(".."))) {
                continue; // don't break here
            };
            char src_path[path_max]; 
            char dst_path[path_max]; 
            snprintf(src_path, sizeof(src_path), "%s/%s", src.data, d_name);
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst.data, d_name);                 
            if (stat(dst_path, &st) == -1) {
                if (!string_contains(String(dst_path), String(".kdl"))) mkdir(dst_path, 0777);   
            }
            recursive_context_copy(st, String(src_path), String(dst_path));
        }
        closedir(damngr_dir);
    } else if (S_ISREG(st.st_mode)) {
        copy_context_file(src, dst);
    }
}

void init_context()
{
    struct stat st;
    const String src = String("damngr"); 
    // TODO: make this path relative to the active user!
    const String dst = String("/home/jappe/.config/damngr");
    if (stat(dst.data, &st) == -1) mkdir(dst.data, 0777);   
    else {
        printf("go to error: config already exists...\n");
        return;
    }

    recursive_context_copy(st, src, dst);
}

//TODO: the context should always validate the current context/state before continuing
// do not put this logic in the parser, only parsing there
Context get_context(Context context)
{
    // TODO: make this path relative to the active user!
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
        char fidbuf[path_max];
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
            char fidbuf[path_max]; 
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

// sudo pacman -Q | cut -d " " -f1
// void execute_get_installed_packages_command(Packages installed_packages) {
//     printf("Executing the get installed packages command...\n");
//     int pipe_fds[2];
//     pipe(pipe_fds);
//     pid_t pid = fork();
//     if (pid == 0) {
//         close(pipe_fds[1]); // close pipe write end child
//         dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
//         close(pipe_fds[0]); // close pipe read end child
//         execl("/usr/bin/sudo", "sudo", "pacman", "-Q", "|", "cut", "-d", " ", "-f1", nullptr);
//     } else {
//         close(pipe_fds[0]); // close pipe read end
//         for (int i = 0; i < (int)packages.count; ++i) {
//             Package package = packages.items[i];
//             if (package.active) {
//                 write(pipe_fds[1], package.item.data, package.item.len); 
//                 write(pipe_fds[1], "\n", 1);
//             }
//         }
//         close(pipe_fds[1]); // close pipe write end
//         waitpid(pid, nullptr, 0);
//     }
// }
//
// void execute_aur_package_install_command(char* path, char* command, Packages aur_packages) {
//     printf("Executing the aur package install command...\n");
//     int pipe_fds[2];
//     pipe(pipe_fds);
//     pid_t pid = fork();
//     if (pid == 0) {
//         close(pipe_fds[1]); // close pipe write end child
//         dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
//         close(pipe_fds[0]); // close pipe read end child
//         execl(path, command, "-Syu", "--needed", "-", nullptr);
//     } else {
//         close(pipe_fds[0]); // close pipe read end
//         for (int i = 0; i < (int)aur_packages.count; ++i) {
//             Package package = aur_packages.items[i];
//             if (package.active) {
//                 write(pipe_fds[1], package.item.data, package.item.len); 
//                 write(pipe_fds[1], "\n", 1);
//             }
//         }
//         close(pipe_fds[1]); // close pipe write end
//         waitpid(pid, nullptr, 0);
//     }
// }

// paru -Qm | cut -d " " -f1
Context get_installed_packages(Context context)
{
    return context;    
}

void free_context(Context context)
{
    memory_cleanup(context.hosts);
    memory_cleanup(context.modules);
    memory_cleanup(context.packages);
    memory_cleanup(context.installed_packages);
    memory_cleanup(context.aur_packages);
    memory_cleanup(context.installed_aur_packages);
    memory_cleanup(context.services);
    memory_cleanup(context.hooks);
    if (context.aur_helper.data != nullptr) {
        free_sized(context.aur_helper.data, context.aur_helper.len);
        context.aur_helper.data = nullptr;
    }
}
