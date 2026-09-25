#ifndef ARRAY_H
#define ARRAY_H
#include <stdlib.h>

constexpr size_t init_capacity = 256;

struct dynamic_array{
    char* elements;
    size_t capacity;
    size_t count;
};

#define DYNAMIC_ARRAY_APPEND(da, e)\
    do {\
        if (da.count >= da.capacity) {\
            if (da.capacity == 0) da.capacity = init_capacity;\
            else da.capacity *= 2;\
            da.elements = realloc(da.elements, da.capacity*sizeof(*da.elements));\
        }\
        da.elements[da.count++] = e;\
    } while(0)

#endif /* ARRAY_H */
