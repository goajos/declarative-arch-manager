#ifndef DAMGR_CMD_H
#define DAMGR_CMD_H
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "damgr/paths.h"
#include "damgr/result.h"
#define SUDO_BIN "/usr/bin/sudo"
#define SYSTEMCTL_BIN "/usr/bin/systemctl"

Result_t damgr_pkg_r(bool install, const char *pkg);
Result_t damgr_aur_install_r(const char *aur_helper, const char *pkg);
Result_t damgr_svc_r(bool enable, bool privileged, const char *svc);
Result_t damgr_dotfile_link_r(const Damgr_Paths *paths, const char *dotfile);
Result_t damgr_dotfile_unlink_r(const Damgr_Paths *paths, const char *dotfile);

// TODO: do we want to have a dry run?
#ifdef DAMGR_CMD_IMPLEMENTATION

static Result_t execute_r(const char *bin, char *const argv[]) {
  fflush(nullptr); // don't duplicate buffered output in the child
  pid_t pid = fork();
  if (pid == -1)
    return Error(damgr_err_fmt("fork failed: %s", strerror(errno)));
  if (pid == 0) {
    execv(bin, argv);
    _exit(127); // only reached if execv failed
  }

  int wstatus;
  while (waitpid(pid, &wstatus, 0) == -1) {
    if (errno != EINTR)
      return Error(damgr_err_fmt("waitpid failed: %s", strerror(errno)));
  }
  if (WIFEXITED(wstatus)) {
    if (WEXITSTATUS(wstatus) == EXIT_SUCCESS)
      return Success;
    return Error(
        damgr_err_fmt("%s exited with code %d", bin, WEXITSTATUS(wstatus)));
  }
  if (WIFSIGNALED(wstatus))
    return Error(damgr_err_fmt("%s was terminated by signal %d", bin,
                               WTERMSIG(wstatus)));
  return Error(
      damgr_err_fmt("%s ended in unexpected state 0x%x", bin, wstatus));
}

Result_t damgr_pkg_r(bool install, const char *pkg) {
  if (install) {
    char *const argv[] = {"sudo",     "pacman",    "-S",
                          "--needed", (char *)pkg, nullptr};
    return execute_r(SUDO_BIN, argv);
  }
  char *const argv[] = {"sudo", "pacman", "-Rn", (char *)pkg, nullptr};
  return execute_r(SUDO_BIN, argv);
}

Result_t damgr_aur_install_r(const char *aur_helper, const char *pkg) {
  char bin[PATH_BUFFER_MAX];
  DAMGR_CALL(damgr_snprintf_r(bin, sizeof(bin), "/usr/bin/%s", aur_helper));
  char *const argv[] = {(char *)aur_helper, "-S", "--needed", (char *)pkg,
                        nullptr};
  return execute_r(bin, argv);
}

// privileged = system service via sudo, otherwise a systemctl --user service
Result_t damgr_svc_r(bool enable, bool privileged, const char *svc) {
  char *action = enable ? "enable" : "disable";
  if (privileged) {
    char *const argv[] = {"sudo", "systemctl", "--now",
                          action, (char *)svc, nullptr};
    return execute_r(SUDO_BIN, argv);
  }
  char *const argv[] = {"systemctl", "--user",    "--now",
                        action,      (char *)svc, nullptr};
  return execute_r(SYSTEMCTL_BIN, argv);
}

// out is true if fid_buffer is a symlink pointing to target
static Result_t is_link_to_r(const char *fid_buffer, const char *target,
                             bool *exists, bool *out) {
  struct stat st;
  *exists = false;
  *out = false;
  if (lstat(fid_buffer, &st) != 0) {
    if (errno == ENOENT)
      return Success;
    return Error(
        damgr_err_fmt("lstat %s failed: %s", fid_buffer, strerror(errno)));
  }
  *exists = true;
  if (!S_ISLNK(st.st_mode))
    return Success;

  char link[PATH_BUFFER_MAX];
  ssize_t len = readlink(fid_buffer, link, sizeof(link) - 1);
  if (len < 0)
    return Error(damgr_err_fmt("could not read link %s: %s", fid_buffer,
                               strerror(errno)));
  link[len] = '\0';
  *out = strcmp(link, target) == 0;
  return Success;
}

Result_t damgr_dotfile_link_r(const Damgr_Paths *paths, const char *dotfile) {
  char src[PATH_BUFFER_MAX];
  char dst[PATH_BUFFER_MAX];
  DAMGR_CALL(damgr_resolve_dotfile_src_r(paths, src, sizeof(src), dotfile));
  DAMGR_CALL(damgr_resolve_dotfile_dst_r(paths, dst, sizeof(dst), dotfile));

  bool exists;
  bool linked;
  DAMGR_CALL(is_link_to_r(dst, src, &exists, &linked));
  if (linked)
    return Success; // already linked
  if (exists)
    return Error(damgr_err_fmt(
        "%s exists and is not a symlink to %s, refusing to overwrite", dst,
        src));

  if (symlink(src, dst) != 0)
    return Error(damgr_err_fmt("could not link %s -> %s: %s", dst, src,
                               strerror(errno)));
  return Success;
}

Result_t damgr_dotfile_unlink_r(const Damgr_Paths *paths, const char *dotfile) {
  char src[PATH_BUFFER_MAX];
  char dst[PATH_BUFFER_MAX];
  DAMGR_CALL(damgr_resolve_dotfile_src_r(paths, src, sizeof(src), dotfile));
  DAMGR_CALL(damgr_resolve_dotfile_dst_r(paths, dst, sizeof(dst), dotfile));

  bool exists;
  bool linked;
  DAMGR_CALL(is_link_to_r(dst, src, &exists, &linked));
  if (!exists)
    return Success; // already unlinked
  if (!linked)
    return Error(damgr_err_fmt("%s is not a symlink to %s, refusing to remove",
                               dst, src));

  if (unlink(dst) != 0)
    return Error(
        damgr_err_fmt("could not unlink %s: %s", dst, strerror(errno)));
  return Success;
}

#endif /* DAMGR_CMD_IMPLEMENTATION */
#endif /* DAMGR_CMD_H */
