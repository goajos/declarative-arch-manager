#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t read_func(void* user_data, char* buf, size_t bufsize) {
    FILE* fid = (FILE* )user_data;
    return fread(buf, 1, bufsize, fid);
}

static size_t write_func(void* user_data, char const* data, size_t nbytes) {
    FILE* fid = (FILE* )user_data;
    return fwrite(data, 1, nbytes, fid);
}

// will be added in glibc 2.43
[[maybe_unused]] static void free_sized(void *ptr, size_t /*size*/)
{
    free(ptr);
}

static char* string_copy(char* str) {
    char* ret = nullptr;
    size_t len = strlen(str); 
    if (len) {
        ret = malloc(len + 1);
        memcpy(ret, str, len);
        ret[len] = '\0'; // ensure proper null termination
    }
    return ret;
}
