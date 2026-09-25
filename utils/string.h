#ifndef STRING_H
#define STRING_H
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t len;
} String;

#define String(x) (String){ x, strlen(x) }

String string_copy(String str);

bool string_equal(String str1, String str2);

bool string_contains(String str1, String str2);

#endif /* STRING_H */
