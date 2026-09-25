#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>

constexpr int PATH_MAX = 4096;

int is_damgr_state_dir_empty(char *dir);

char *get_user();

size_t read_func(void *user_data, char *buf, size_t bufsize);

char *string_copy(char *str);
bool string_contains(char *haystack, char *needle);

int qcharcmp(const void *p1, const void *p2);

#endif /* UTILS_H */
