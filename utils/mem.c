#include <stdlib.h>
#include <stdio.h>

# define memory_cleanup(xs)\
    for (int i = 0; i < (int)xs.count; ++i) {\
        String item = xs.items[i].item;\
        free_sized(item.data, item.len);\
        item.data = nullptr;\
    }\
    free_sized(xs.items, xs.capacity*sizeof(*modules.items));\
    xs.items = nullptr;

// will be added in glibc 2.43
void free_sized(void *ptr, size_t /*size*/)
{
    free(ptr);
}

static size_t read_func(void *user_data, char *buf, size_t bufsize)
{
    FILE *fid = (FILE *)user_data;
    return fread(buf, 1, bufsize, fid);
}

