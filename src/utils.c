#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
