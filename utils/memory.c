#include <stdlib.h>
#include <string.h>

char* string_copy(char* str) {
    char* ret = nullptr;
    size_t len = strlen(str); 
    if (len) {
        ret = malloc(len + 1);
        memcpy(ret, str, len);
        ret[len] = '\0'; // ensure proper null termination
    }
    return ret;
}

// will be added in glibc 2.43
void free_sized(void *ptr, size_t /*size*/)
{
    free(ptr);
}
