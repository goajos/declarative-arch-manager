#include <stdlib.h>
#include "string.h"

String string_copy(String str)
{
    String dst = { .data = nullptr };
    if (str.len) {
        char *data = (char *)malloc(str.len + 1);
        dst.data = data;
        dst.len = str.len;
        memcpy(dst.data, str.data, str.len);
        dst.data[dst.len] = '\0'; // ensure the String is properly terminated
    }
    return dst;
}

bool string_equal(String str1, String str2)
{
    if (str1.len != str2.len) {
        return false;
    }
    return memcmp(str1.data, str2.data, str1.len) == 0;
}

bool string_contains(String str1, String str2)
{
    bool contains = false;
    for (size_t i = 0, j = 0; i < str1.len && !contains; ++i) {
        while (str1.data[i] == str2.data[j]) {
            ++j;
            ++i;
            if (j == str2.len) {
                contains = true;
                return contains;
            }
        }
        j = 0;
    }
    return contains;
}
