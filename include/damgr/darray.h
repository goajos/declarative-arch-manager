#ifndef DAMGR_DARRAY_H
#define DAMGR_DARRAY_H
#include <stdlib.h>

typedef struct darray {
  void **ptrs;
  size_t capacity;
  size_t count;
} Damgr_Darray;
void damgr_darray_append(Damgr_Darray *darray, void *ptr);
void damgr_darray_free(Damgr_Darray *darray);

#endif /* DAMGR_DARRAY_H */
