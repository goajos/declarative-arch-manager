#define _DEFAULT_SOURCE
#include "damgr/cmd.h"

int execute_execv(const char *fid_buf, char *const argv[]) {
  pid_t pid = fork();
  if (pid == -1) {
    damgr_log(ERROR, "fork failed: %s", strerror(errno));
    return EXIT_FAILURE;
  } else if (pid == 0) {
    execv(fid_buf, argv);
    // only reached if execv failed
    _exit(EXIT_FAILURE);
  } else {
    int wstatus;
    if (waitpid(pid, &wstatus, 0) != pid) {
      damgr_log(ERROR, "waitpid failed: %s", strerror(errno));
      return EXIT_FAILURE;
    }
    if (WIFEXITED(wstatus)) {
      if (WEXITSTATUS(wstatus) != EXIT_SUCCESS) {
        damgr_log(ERROR, "child process for %s failed with exit code: %d",
                  fid_buf, WEXITSTATUS(wstatus));
        return EXIT_FAILURE;
      }
    } else if (WIFSIGNALED(wstatus)) {
      damgr_log(ERROR, "child process for %s was terminated by signal: %d",
                fid_buf, WTERMSIG(wstatus));
      return EXIT_FAILURE;
    } else {
      damgr_log(ERROR, "child process for %s ended in unexpected state: 0x%x",
                fid_buf, wstatus);
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int run_package_command(const bool install, char *const *names,
                        const size_t cnt) {
  char **argv = malloc((cnt + (install ? 5 : 4)) * sizeof(char *));
  if (argv == nullptr) {
    damgr_log(ERROR, "malloc failed for package %s command argv",
              install ? "install" : "remove");
    return EXIT_FAILURE;
  }
  argv[0] = "sudo";
  argv[1] = "pacman";
  if (install) {
    argv[2] = "-S";
    argv[3] = "--needed";
    for (size_t i = 0; i < cnt; ++i)
      argv[4 + i] = names[i];
    argv[4 + cnt] = nullptr;
  } else {
    // argv[2] = "-Rns"; // also removes dependencies?
    argv[2] = "-Rn";
    for (size_t i = 0; i < cnt; ++i)
      argv[3 + i] = names[i];
    argv[3 + cnt] = nullptr;
  }
  int ret = execute_execv("/usr/bin/sudo", argv);
  free(argv);
  return ret;
}

int run_aur_install_command(const char *helper, char *const *names,
                            size_t cnt) {
  char fid_buf[DAMGR_FID_BUF_MAX];
  snprintf(fid_buf, sizeof(fid_buf), "/usr/bin/%s", helper);
  char **argv = malloc((cnt + 4) * sizeof(char *));
  if (argv == nullptr) {
    damgr_log(ERROR, "malloc failed for aur install command argv");
    return EXIT_FAILURE;
  }
  argv[0] = (char *)helper;
  argv[1] = "-S";
  argv[2] = "--needed";
  for (size_t i = 0; i < cnt; ++i)
    argv[3 + i] = names[i];
  argv[3 + cnt] = nullptr;
  int ret = execute_execv(fid_buf, argv);
  free(argv);
  return ret;
}

int run_service_command(const bool privileged, const bool to_enable,
                        const char *service) {
  char **argv = malloc(6 * sizeof(char *));
  if (argv == nullptr) {
    damgr_log(ERROR, "malloc failed for %s %s service command argv",
              to_enable ? "enable" : "disable", privileged ? "root" : "user");
    return EXIT_FAILURE;
  }
  if (privileged) {
    argv[0] = "sudo";
    argv[1] = "systemctl";
    argv[2] = "--now";
    argv[3] = to_enable ? "enable" : "disable";
    argv[4] = (char *)service;
    argv[5] = nullptr;
  } else {
    argv[0] = "systemctl";
    argv[1] = "--user";
    argv[2] = "--now";
    argv[3] = to_enable ? "enable" : "disable";
    argv[4] = (char *)service;
    argv[5] = nullptr;
  }
  int ret =
      execute_execv(privileged ? "/usr/bin/sudo" : "/usr/bin/systemctl", argv);
  free(argv);
  return ret;
}

// TODO: can we use the format function here?
int do_dotfile_link(const char *dotfile) {
  char src[DAMGR_FID_BUF_MAX];
  char dst[DAMGR_FID_BUF_MAX];
  snprintf(src, sizeof(src), "%s/.config/damgr/dotfiles/%s", getenv("HOME"),
           dotfile);
  snprintf(dst, sizeof(dst), "%s/.config/%s", getenv("HOME"), dotfile);

  struct stat st;
  if (lstat(dst, &st) == 0) {
    if (S_ISLNK(st.st_mode)) {
      return EXIT_SUCCESS; // already linked, idempotent
    }
    damgr_log(ERROR,
              "dotfile destination exists and is not a symlink, refusing to "
              "overwrite: %s",
              dst);
    return EXIT_FAILURE;
  }
  if (symlink(src, dst) != 0) {
    damgr_log(ERROR, "could not symlink dotfile %s -> %s: %s", src, dst,
              strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// TODO: can we use the format function here?
int do_dotfile_unlink(const char *dotfile) {
  char dst[DAMGR_FID_BUF_MAX];
  snprintf(dst, sizeof(dst), "%s/.config/%s", getenv("HOME"), dotfile);
  if (unlink(dst) != 0 && errno != ENOENT) {
    damgr_log(ERROR, "could not unlink dotfile %s: %s", dst, strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
