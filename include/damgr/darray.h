#ifndef DAMGR_DARRAY_H
#define DAMGR_DARRAY_H
#include <stdlib.h>

typedef struct darray {
  char **items;
  size_t capacity;
  size_t count;
} Damgr_Darray;
void damgr_darray_append(Damgr_Darray *darray, char *item);

#endif /* DAMGR_DARRAY_H */
