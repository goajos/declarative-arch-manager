#include "damgr/darray.h"

void damgr_darray_append(Damgr_Darray *darray, char *item) {
  if (darray->count >= darray->capacity) {
    if (darray->capacity == 0) {
      darray->capacity = 16;
    } else {
      darray->capacity *= 2;
    }
    darray->items =
        realloc(darray->items, darray->capacity * sizeof(*darray->items));
  }
  darray->items[darray->count++] = item;
}
