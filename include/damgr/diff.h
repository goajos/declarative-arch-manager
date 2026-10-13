#ifndef DAMGR_DIFF_H
#define DAMGR_DIFF_H
#include "damgr/state.h"

typedef enum diff_type { ADD, REMOVE, CHANGE, SKIP } Damgr_Diff_Type;

typedef struct array_diff {
  const char **state_array;
  size_t state_cnt;
  const char **active_array;
  size_t active_cnt;
  bool *only_in_state;
  bool *only_in_active;
} Damgr_Array_Diff;
int damgr_get_diff_array(Damgr_Array_Diff *diff, Damgr_Arena *arena);

typedef struct module_diff {
  Damgr_Diff_Type diff_type;
  Damgr_Module *state_module;
  Damgr_Module *active_module;
  Damgr_Array_Diff packages;
  Damgr_Array_Diff aur_packages;
  Damgr_Array_Diff root_services;
  Damgr_Array_Diff user_services;
  bool to_link_diff;
} Damgr_Module_Diff;
int damgr_get_diff_module(Damgr_Module_Diff *module_diff,
                          Damgr_Module *state_module,
                          Damgr_Module *active_module, Damgr_Arena *arena);

typedef struct host_diff {
  Damgr_Module_Diff *diffs;
  size_t diff_cnt;
  bool host_changed;
} Damgr_Host_Diff;
int damgr_get_diff_host(Damgr_Host_Diff *host_diff, Damgr_Host *state_host,
                        Damgr_Host *active_host, Damgr_Arena *arena);

#endif /* DAMGR_DIFF_H */
