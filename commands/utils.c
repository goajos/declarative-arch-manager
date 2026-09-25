#include <dirent.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../state/state.h"
 
char* get_user() {
    struct passwd* pwd = getpwuid(geteuid());
    return pwd->pw_name;
}

int copy_file(char* src, char* dst)
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

int recursive_init_state(struct stat st, char* src, char* dst)
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
            // check if the new src is a directory
            stat(src_fidbuf, &st);
            if (S_ISDIR(st.st_mode)) {
                // stats returns -1 if the path doesn't exist
                if (stat(dst_fidbuf, &st) == -1) {
                    // ensure the dst directory exists
                    mkdir(dst_fidbuf, 0777);    
                }
            }
            ret = recursive_init_state(st, src_fidbuf, dst_fidbuf);
        }
        closedir(damngr_dir);
    } else if (S_ISREG(st.st_mode)) { // src is a file
        ret = copy_file(src, dst);
        if (strstr(dst, "hook") != nullptr) chmod(dst, 0777); // ensure hooks are executable
    }

    return ret;
}
int execute_package_remove_command(struct dynamic_array packages) {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", "pacman", "-Rs", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (size_t i = 0; i < packages.count; ++i) {
            char* package = packages.items[i];
            write(pipe_fds[1], package, strlen(package)); 
            write(pipe_fds[1], "\n", 1);
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

int execute_package_install_command(struct dynamic_array packages) {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl("/usr/bin/sudo", "sudo", "pacman", "-S", "--needed", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (size_t i = 0; i < packages.count; ++i) {
            char* package = packages.items[i];
            write(pipe_fds[1], package, strlen(package)); 
            write(pipe_fds[1], "\n", 1);
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

int execute_aur_package_install_command(char* fid,
                                        char* command,
                                        struct dynamic_array packages) {
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        close(pipe_fds[1]); // close pipe write end child
        dup2(pipe_fds[0], STDIN_FILENO); // redirect to pipe read end child 
        close(pipe_fds[0]); // close pipe read end child
        execl(fid, command, "-S", "--needed", "-", nullptr);
    } else {
        close(pipe_fds[0]); // close pipe read end
        for (size_t i = 0; i < packages.count; ++i) {
            char* package = packages.items[i];
            write(pipe_fds[1], package, strlen(package)); 
            write(pipe_fds[1], "\n", 1);
        }
        close(pipe_fds[1]); // close pipe write end
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

int execute_service_command(bool privileged, bool enable, struct dynamic_array services) {
    for (size_t i = 0; i < services.count; ++i) {
        pid_t pid = fork();
        if (pid == -1) return EXIT_FAILURE;
        if (pid == 0) {
            if (privileged) {
                char* argv[] = { "sudo",
                            "systemctl",
                            (enable) ? "enable" : "disable",
                            services.items[i],
                            nullptr };
                execv("/usr/bin/sudo", argv);
            } else {
                char* argv[] = { "systemctl",
                            "--user",
                            (enable) ? "enable" : "disable",
                            services.items[i],
                            nullptr };
                execv("/usr/bin/systemctl", argv);
            }
        } else {
            waitpid(pid, nullptr, 0);
        }
    }
    return EXIT_SUCCESS;
}

static int link_dotfiles(bool link, char* src, char* dst) {
    pid_t pid = fork();
    if (pid == -1) return EXIT_FAILURE;
    if (pid == 0) {
        if (link) {
            execl("/usr/bin/ln", "ln", "--symbolic", src, dst, nullptr);
        } else {
            execl("/usr/bin/rm", "rm", dst, nullptr);
        }
    } else {
        waitpid(pid, nullptr, 0);
    }
    return EXIT_SUCCESS;
}

int execute_dotfile_link_command(bool link, struct dynamic_array dotfiles) {
    int ret;
    struct stat st;
    for (size_t i = 0; i < dotfiles.count; ++i) {
        char src_fidbuf[path_max];
        snprintf(src_fidbuf,
                sizeof(src_fidbuf),
                "/home/%s/.config/damngr/dotfiles/%s",
                get_user(),
                dotfiles.items[i]); 
        stat(src_fidbuf, &st);
        if (S_ISDIR(st.st_mode)) {
            char dst_fidbuf[path_max];
            snprintf(dst_fidbuf,
                    sizeof(dst_fidbuf),
                    "/home/%s/.config/%s",
                    get_user(),
                    dotfiles.items[i]);
            ret = link_dotfiles(link, src_fidbuf, dst_fidbuf);
        } else return EXIT_FAILURE;
    }
    return ret;
}

int execute_hook_command(bool privileged, struct dynamic_array hooks) {
    for (size_t i = 0; i < hooks.count; ++i) {
        pid_t pid = fork();
        if (pid == -1) return EXIT_FAILURE;
        if (pid == 0) {
            char fidbuf[path_max];
            snprintf(fidbuf,
                    sizeof(fidbuf),
                    "/home/%s/.config/damngr/hooks/%s",
                    get_user(),
                    hooks.items[i]);
            if (privileged) {
                execl("/usr/bin/sudo", "sudo", fidbuf, nullptr);
            } else {
                execl(fidbuf, hooks.items[i], nullptr);
            }
        } else {
            waitpid(pid, nullptr, 0);
        }
    }
    return EXIT_SUCCESS;
}
