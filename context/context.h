#ifndef CONTEXT_H
#define CONTEXT_H
#include <stdlib.h>
#include "../utils/array.h"

constexpr size_t path_max = 4096;

extern int init_context();

extern int get_aur_helper(char** aur_helper);

extern int get_sync_context(char** aur_helper, struct active_dynamic_array* packages, struct active_dynamic_array* aur_packages, struct active_dynamic_array* root_services, struct active_dynamic_array* root_hooks, struct active_dynamic_array* user_services, struct active_dynamic_array* user_hooks);

#endif /* CONTEXT_H */
