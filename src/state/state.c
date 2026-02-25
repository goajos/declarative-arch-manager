#include "damgr/state.h"

void darray_append(struct darray *array, char *item) {
  if (array->count >= array->capacity) {
    if (array->capacity == 0) {
      array->capacity = 16;
    } else {
      array->capacity *= 2;
    }
    array->items =
        realloc(array->items, array->capacity * sizeof(*array->items));
  }
  array->items[array->count++] = item;
}

void modules_append(struct modules *modules, struct module module) {
  if (modules->count >= modules->capacity) {
    if (modules->capacity == 0) {
      modules->capacity = 16;
    } else {
      modules->capacity *= 2;
    }
    modules->items =
        realloc(modules->items, modules->capacity * sizeof(*modules->items));
  }
  modules->items[modules->count++] = module;
}
