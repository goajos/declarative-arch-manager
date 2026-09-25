#include "damgr/utils.h"
#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int is_damgr_state_dir_empty(char *dir) {
  DIR *open_dir = opendir(dir);
  if (open_dir == nullptr) {
    return EXIT_FAILURE;
  }

  int cnt = 0;
  struct dirent *ent;
  while ((ent = readdir(open_dir)) != nullptr) {
    ++cnt;
  }
  closedir(open_dir);
  return cnt;
}

char *get_user() {
  struct passwd *pwd = getpwuid(geteuid());
  return pwd->pw_name;
}

size_t read_func(void *user_data, char *buf, size_t bufsize) {
  FILE *fid = (FILE *)user_data;
  return fread(buf, 1, bufsize, fid);
}

char *string_copy(char *str) {
  char *ret = nullptr;
  size_t len = strlen(str);
  if (len) {
    ret = malloc(len + 1);
    memcpy(ret, str, len);
    ret[len] = '\0'; // ensure proper null termination
  }
  return ret;
}
