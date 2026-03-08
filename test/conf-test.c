#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

bool string_contains(char *haystack, char *needle) {
  bool contains = false;
  for (size_t i = 0, j = 0; i < strlen(haystack) && !contains; ++i) {
    while (haystack[i] == needle[j]) {
      ++j;
      ++i;
      if (j == strlen(needle)) {
        contains = true;
        return contains;
      }
    }
    j = 0;
  }
  return contains;
}

void string_trim(char *str) {
  if (*str == '\0' || *str == '\n') {
    return;
  }
  char *start = str;
  while (isspace((char)*start)) {
    ++start;
  }
  char *end = str + strlen(str) - 1;
  while (end > start && isspace((char)*end)) {
    --end;
  }
  *(end + 1) = '\0'; // ensure proper null termination

  if (start != str) {
    memmove(str, start, end - start + 2); // +2 includes the null terminator
  }
}

// TODO: use this Camelcase for all enums and structs?
enum Config_key {
  AUR_HELPER,
  ACTIVE_HOST,
  MODULES,
  SERVICES,
  DOTFILES,
  PACKAGES,
  PRE_HOOKS,
  POST_HOOKS,
  MAX_CONFIG_KEY,
};
const char *config_keys[] = {
    [AUR_HELPER] = "aur_helper", [ACTIVE_HOST] = "active_host",
    [MODULES] = "modules",       [SERVICES] = "services",
    [DOTFILES] = "dotfiles",     [PACKAGES] = "packages",
    [PRE_HOOKS] = "pre_hooks",   [POST_HOOKS] = "post_hooks"};

int get_config_key(char *name) {
  for (int i = 0; i < MAX_CONFIG_KEY; ++i) {
    if (strcmp(config_keys[i], name) == 0) {
      return i;
    }
  }
  return -1;
}

struct Darray {
  char **items;
  size_t capacity;
  size_t count;
};

void darray_append(struct Darray *darray, char *item) {
  if (darray->count >= darray->capacity) {
    if (darray->capacity == 0) {
      darray->capacity = 16;
    } else {
      darray->capacity *= 2;
    }
    darray->items =
        realloc(darray->items, darray->capacity * sizeof(*darray->items));
  }
  darray->items[darray->count++] = item;
}

struct Host {
  struct Darray modules;
  struct Darray services;
  char *name;
};

struct Config {
  struct Host active_host;
  char *aur_helper;
};

void parse_line(int *config_key, struct Config *config, char *line) {
  if (string_contains(line, "=")) {
    char key[256];
    char value[256];
    if (sscanf(line, "%255[^=]=%255[^\n]", key, value) != 2) {
      // TODO: damgr_log(ERROR, "sscanf failed for line: %s", line);
    }
    string_trim(key);
    *config_key = get_config_key(key);
    string_trim(value);
    if (value[0] != '\0') {
      switch (*config_key) {
      case AUR_HELPER:
        config->aur_helper = string_copy(value);
        break;
      case ACTIVE_HOST:
        config->active_host.name = string_copy(value);
        break;
      }
      puts("");
    }
    printf("%s", line);
  } else {
    string_trim(line);
    if (line[0] == '#' || line[0] == '\n') {
      return;
    }
    printf("config key: %d\n", *config_key);
    switch (*config_key) {
    case MODULES:
      darray_append(&config->active_host.modules, string_copy(line));
      break;
    case SERVICES:
      darray_append(&config->active_host.services, string_copy(line));
      break;
    }
    printf("%s\n", line);
  }
}

// gcc -g -o conf-test.o conf-test.c
int main() {
  FILE *config_fid = fopen("config.conf", "r");
  FILE *host_fid = fopen("arch.conf", "r");

  int config_key;
  struct Config config = {};
  char line[512];
  size_t lines = 0;
  while (fgets(line, sizeof(line), config_fid)) {
    ++lines;
    if (line[0] == '#') {
      continue;
    }
    parse_line(&config_key, &config, line);
  }
  fclose(config_fid);
  while (fgets(line, sizeof(line), host_fid)) {
    ++lines;
    if (line[0] == '#') {
      continue;
    }
    parse_line(&config_key, &config, line);
  }
  fclose(host_fid);
  return 0;
}
