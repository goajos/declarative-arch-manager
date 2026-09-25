#ifndef  STATE_UTILS_H
#define  STATE_UTILS_H
#include <stdlib.h>

size_t read_func(void* user_data, char* buf, size_t bufsize);
size_t write_func(void* user_data, char const* data, size_t nbytes);

char* string_copy(char* str);
// will be added in glibc 2.43
void free_sized(void* ptr, size_t /*size*/);

int qcharcmp(const void* p1, const void* p2);
int qnamecmp(const void* p1, const void* p2);

#endif /* STATE_UTILS_H */
