#ifndef MEMORY_H
#define MEMORY_H
#include <stdlib.h>

char* string_copy(char* str);

// will be added in glibc 2.43
void free_sized(void *ptr, size_t /*size*/);

#endif /* MEMORY_H */
