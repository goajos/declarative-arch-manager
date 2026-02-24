#include "utils.h"
#include "state.h"
#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void darray_append(struct darray *array, char *item) {
  if (array->count >= array->capacity) {
    if (array->capacity == 0) {
      array->capacity = 16;
    } else {
      array->capacity *= 2;
    }
    array->items =
        realloc(array->items, array->capacity * sizeof(*array->items));
  }
  array->items[array->count++] = item;
}

void modules_append(struct modules *modules, struct module module) {
  if (modules->count >= modules->capacity) {
    if (modules->capacity == 0) {
      modules->capacity = 16;
    } else {
      modules->capacity *= 2;
    }
    modules->items =
        realloc(modules->items, modules->capacity * sizeof(*modules->items));
  }
  modules->items[modules->count++] = module;
}

int is_state_dir_empty(char *dir) {
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
