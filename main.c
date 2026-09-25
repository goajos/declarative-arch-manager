#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <glob.h>
#include "kdl_parsing.c"
#include "utils/debug.c"

void execute_package_install_command(char* command, Packages packages) {
    printf("Executing the package command...\n");
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", command, "-Syu", "--needed", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (int i = 0; i < (int)packages.count; ++i) {
            Package package = packages.items[i];
            if (package.active) {
                write(pipe_fds[1], package.item.data, package.item.len); 
                write(pipe_fds[1], "\n", 1);
            }
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
}

// TODO: does not require sudo!!!
void execute_aur_package_install_command(char* command, AurPackages aur_packages) {
    printf("Executing the aur package command...\n");
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", command, "-Syu", "--needed", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (int i = 0; i < (int)aur_packages.count; ++i) {
            Package package = aur_packages.items[i];
            if (package.active) {
                write(pipe_fds[1], package.item.data, package.item.len); 
                write(pipe_fds[1], "\n", 1);
            }
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
}

// clang main.c -o main -g -lc -lkdl -lm -std=c23 -Wall -Wextra -Werror -pedantic
int main()
{
    Hosts hosts = {0};

    FILE *config_fid = fopen("/home/jappe/.config/damngr/config.kdl", "r");
    parse_config_kdl(config_fid, &hosts);
    fclose(config_fid);

    printf("hosts count: %d\n", (int)hosts.count);
    String active_host = { .data = nullptr };
    for (int i = 0; i < (int)hosts.count; ++i) {
        if (hosts.items[i].active) {
           active_host = String(hosts.items[i].item.data);
           break; // only 1 active host assumption
        }
    }
    // TODO: no active host exit
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
    
    Modules modules = {0};
    Services services = {0};
    Hooks hooks = {0};
    // if an active host is found
    if (host_path.data != nullptr) {
        char fidbuf[0x4096]; // TODO: magic number for max path length? 
        printf("%s\n", host_path.data);
        snprintf(fidbuf, sizeof(fidbuf), "%s", host_path.data);
        FILE *host_fid = fopen(fidbuf, "r");  
        parse_host_kdl(host_fid, &modules, &services, &hooks);
        fclose(host_fid);
        
        debug_active(modules)
    } else return 1;
    // no longer need hosts and host glob
    memory_cleanup(hosts);
    free_sized(host_path.data, host_path.len);
    globfree(&host_globbuf);

    Packages packages = {0};
    AurPackages aur_packages = {0};
    for (int i = 0; i < (int)modules.count; ++i) {
        Module module = modules.items[i];
        if (module.active) {
            char fidbuf[0x4096]; // TODO: magic number for max path length? 
            snprintf(fidbuf, sizeof(fidbuf), "/home/jappe/.config/damngr/modules/%s.kdl", module.item.data);
            printf("%s\n", fidbuf);
            FILE *module_fid = fopen(fidbuf, "r");
            parse_module_kdl(module_fid, &packages, &aur_packages, &services, &hooks);
            fclose(module_fid);
        }

    }
    debug_active(packages);
    debug_active(aur_packages);
    debug_active_user_type(services);
    
    execute_package_install_command("pacman", packages);
    execute_aur_package_install_command("paru", aur_packages);

    memory_cleanup(modules);
    memory_cleanup(packages);
    memory_cleanup(services);
    memory_cleanup(hooks);
    
    return 0;
}
