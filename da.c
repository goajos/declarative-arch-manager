// dynamic arrays
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *packages;
    size_t count;
    size_t capacity;
} Packages;

int main(void)
{
    Packages ps = {0};
    for (int p = 0; p < 10; ++p) {
        if (ps.count >= ps.capacity) {
            if (ps.capacity == 0) ps.capacity = 2;
            else ps.capacity *= 2; 
            ps.packages = realloc(ps.packages, ps.capacity*sizeof(*ps.packages));
        }
        printf("Packages capacity: %ld\n", ps.capacity);
        ps.packages[ps.count++] = p;
    }
    for (size_t i = 0; i < ps.count; ++i) {
        printf("%d\n", ps.packages[i]);
    }
    return 0;
}
