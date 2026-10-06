#include "damgr/darray.h"

void damgr_darray_append(Damgr_Darray *darray, void *ptr) {
  if (darray->count >= darray->capacity) {
    if (darray->capacity == 0) {
      darray->capacity = 16;
    } else {
      darray->capacity *= 2;
    }
    darray->ptrs =
        realloc(darray->ptrs, darray->capacity * sizeof(*darray->ptrs));
  }
  darray->ptrs[darray->count++] = ptr;
}

void damgr_darray_free(Damgr_Darray *darray) {
  for (size_t i = 0; i < darray->count; ++i) {
    free(darray->ptrs[i]);
  }
  if (darray != nullptr)
    free(darray->ptrs);
}
