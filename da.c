// dynamic arrays
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *items;
    size_t count;
    size_t capacity;
} Packages;

#define da_append(xs, x)\
    do {\
        if (xs.count >= xs.capacity) {\
            if (xs.capacity == 0) xs.capacity = 256;\
            else xs.capacity *= 2;\
            xs.items = realloc(xs.items, xs.capacity*sizeof(*xs.items));\
        }\
        xs.items[xs.count++] = x;\
    } while(0)

int main(void)
{
    Packages xs = {0};
    for (int x = 0; x < 10; ++x) da_append(xs, x);
    for (int i = 0; i < 10; ++i) printf("%d\n", xs.items[i]); 
    return 0;
}
