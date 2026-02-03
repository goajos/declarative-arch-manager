#ifndef CONTEXT_H
#define CONTEXT_H
#include <stdlib.h>

constexpr size_t path_max = 4096;

extern int init_context();

extern int get_aur_helper(char** aur_helper);

#endif /* CONTEXT_H */
