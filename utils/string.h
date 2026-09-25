#ifndef STRING_H
#define STRING_H
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char *data;
    size_t len;
} String;

#define String(x) (String){ x, strlen(x) }

extern String string_copy(String str);

extern bool string_equal(String str1, String str2);

extern bool string_contains(String str1, String str2);

#endif /* STRING_H */
