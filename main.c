#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DYNAMIC_ARRAY_APPEND(da, item)\
    do {\
        if (da.count >= da.capacity) {\
            if (da.capacity == 0) da.capacity = 10;\
            else da.capacity *= 2;\
            da.items = realloc(da.items, da.capacity*sizeof(*da.items));\
        }\
        da.items[da.count++] = item;\
    } while(0)

static int qstrcmp(const void *p1, const void *p2) {
    return strcmp(*(const char** )p1, *(const char** )p2);
}

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

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    char* p1[] = { "pacman", "decman", "sudo", "less", "dcli" };
    size_t p1_cnt = 5;
    char* p2[] = { "pacman", "damngr", "sudo", "man-db", "manpages", "dcli"};
    size_t p2_cnt = 6;

    struct dynamic_array{
        char** items;
        size_t capacity;
        size_t count;
    }; 
    struct dynamic_array to_remove = { };
    struct dynamic_array to_install = { };
    struct dynamic_array to_keep = { };


    size_t i = 0;
    size_t j = 0;
    qsort(p1, p1_cnt, sizeof(p1[0]), qstrcmp);
    qsort(p2, p2_cnt, sizeof(p2[0]), qstrcmp);
    while (i < p1_cnt && j < p2_cnt) {
        int res = strcmp(p1[i], p2[j]);
        if (res < 0) { // p1 < p2
            DYNAMIC_ARRAY_APPEND(to_remove, p1[i]); 
            ++i;
        } else if (res > 0) { // p1 > p2
            DYNAMIC_ARRAY_APPEND(to_install, p2[j]); 
            ++j;
        } else { // packages are equal
            DYNAMIC_ARRAY_APPEND(to_keep, p1[i]); 
            ++i;
            ++j;
        }
    }
    while (i < p1_cnt) {
        DYNAMIC_ARRAY_APPEND(to_remove, p1[i]); 
        ++i;
    }
    while (j < p2_cnt) {
        DYNAMIC_ARRAY_APPEND(to_install, p2[j]); 
        ++j;
    }

    puts("to remove:");
    for (size_t i = 0; i < to_remove.count; ++i) {
        printf("%s\n", to_remove.items[i]);
    }
    puts("to install:");
    for (size_t i = 0; i < to_install.count; ++i) {
        printf("%s\n", to_install.items[i]);
    }
    puts("to keep:");
    for (size_t i = 0; i < to_keep.count; ++i) {
        printf("%s\n", to_keep.items[i]);
    }

    return EXIT_SUCCESS;
}
