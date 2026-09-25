#ifndef DAMGR_UTILS_H
#define DAMGR_UTILS_H
#include "damgr/state.h"
#include <stdlib.h>

extern const int damgr_path_max;

int damgr_is_state_dir_empty(char *dir);
int damgr_init_dir(char *user, bool is_state);

char *damgr_get_user();

char *damgr_string_copy(char *str);

bool damgr_string_contains(char *haystack, char *needle);

void damgr_string_trim(char *str);

int damgr_get_conf_key(char *key);

int damgr_execute_aur_update_command(char *aur_helper);
int damgr_execute_update_command();

#endif /* DAMGR_UTILS_H */
