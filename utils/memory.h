#ifndef MEMORY_H
#define MEMORY_H
#include <stdlib.h>

char* string_copy(char* str);

// will be added in glibc 2.43
void free_sized(void *ptr, size_t /*size*/);

#define DYNAMIC_ARRAY_FREE(da)\
    for (size_t i = 0; i < da.count; ++i) {\
        char* elem = da.elements[i].element;\
        free_sized(elem, strlen(elem));\
        elem = nullptr;\
    }\
    free_sized(da.elements, da.capacity*sizeof(*da.elements));\
    da.elements = nullptr;

#endif /* MEMORY_H */
