#include "damgr/diff.h"
#include "damgr/state.h"

static bool in_array(const char **array, size_t cnt, const char *item) {
  for (size_t i = 0; i < cnt; ++i) {
    if (strcmp(array[i], item) == 0) {
      return true;
    }
  }
  return false;
}

static void *find_module(Damgr_Module **modules, size_t modules_cnt,
                         const char *module_name) {
  for (size_t i = 0; i < modules_cnt; ++i) {
    if (strcmp(modules[i]->module_name, module_name) == 0) {
      return modules[i];
    }
  }
  return nullptr;
}

static bool has_changed(Damgr_Array_Diff *array_diff) {
  for (size_t i = 0; i < array_diff->active_cnt; ++i)
    if (array_diff->only_in_active[i])
      return true;
  for (size_t i = 0; i < array_diff->state_cnt; ++i)
    if (array_diff->only_in_state[i])
      return true;
  return false;
}

void damgr_get_diff_array(Damgr_Array_Diff *diff, Damgr_Arena *arena) {
  diff->only_in_active = arena_alloc(arena, diff->active_cnt);
  diff->only_in_state = arena_alloc(arena, diff->state_cnt);
  for (size_t i = 0; i < diff->active_cnt; ++i) {
    diff->only_in_active[i] =
        !in_array(diff->state_array, diff->state_cnt, diff->active_array[i]);
  }
  for (size_t i = 0; i < diff->state_cnt; ++i) {
    diff->only_in_state[i] =
        !in_array(diff->active_array, diff->active_cnt, diff->state_array[i]);
  }
}

void damgr_get_diff_module(Damgr_Module_Diff *module_diff,
                           Damgr_Module *state_module,
                           Damgr_Module *active_module, Damgr_Arena *arena) {
  module_diff->diff_type = CHANGE;
  module_diff->state_module = state_module;
  module_diff->active_module = active_module;
  // TODO: does this cover all to_link cases?
  module_diff->to_link_diff = state_module->to_link != active_module->to_link;

  module_diff->packages.state_array = state_module->packages;
  module_diff->packages.state_cnt = state_module->packages_cnt;
  module_diff->packages.active_array = active_module->packages;
  module_diff->packages.active_cnt = active_module->packages_cnt;
  damgr_get_diff_array(&module_diff->packages, arena);

  module_diff->aur_packages.state_array = state_module->aur_packages;
  module_diff->aur_packages.state_cnt = state_module->aur_packages_cnt;
  module_diff->aur_packages.active_array = active_module->aur_packages;
  module_diff->aur_packages.active_cnt = active_module->aur_packages_cnt;
  damgr_get_diff_array(&module_diff->aur_packages, arena);

  module_diff->root_services.state_array = state_module->root_services;
  module_diff->root_services.state_cnt = state_module->root_services_cnt;
  module_diff->root_services.active_array = active_module->root_services;
  module_diff->root_services.active_cnt = active_module->root_services_cnt;
  damgr_get_diff_array(&module_diff->root_services, arena);

  module_diff->user_services.state_array = state_module->user_services;
  module_diff->user_services.state_cnt = state_module->user_services_cnt;
  module_diff->user_services.active_array = active_module->user_services;
  module_diff->user_services.active_cnt = active_module->user_services_cnt;
  damgr_get_diff_array(&module_diff->user_services, arena);

  // downgrade type to SKIP if no changes
  bool any = module_diff->to_link_diff;
  any = any || has_changed(&module_diff->packages) ||
        has_changed(&module_diff->aur_packages) ||
        has_changed(&module_diff->root_services) ||
        has_changed(&module_diff->user_services);
  if (!any)
    module_diff->diff_type = SKIP;
}

void damgr_get_diff_host(Damgr_Host_Diff *host_diff, Damgr_Host *state_host,
                         Damgr_Host *active_host, Damgr_Arena *arena) {
  host_diff->host_changed =
      state_host == nullptr ||
      strcmp(state_host->host_name, active_host->host_name) != 0;

  size_t max_cnt =
      active_host->modules_cnt + (state_host ? state_host->modules_cnt : 0);
  host_diff->diffs = arena_alloc(arena, max_cnt * sizeof(Damgr_Module_Diff));
  host_diff->diff_cnt = 0;

  // pass 1: walk active modules -> ADD or CHANGE or SKIP
  for (size_t i = 0; i < active_host->modules_cnt; ++i) {
    Damgr_Module *active_module = active_host->modules[i];
    Damgr_Module *state_module =
        state_host ? find_module(state_host->modules, state_host->modules_cnt,
                                 active_module->module_name)
                   : nullptr;
    Damgr_Module_Diff *module_diff = &host_diff->diffs[host_diff->diff_cnt++];
    if (state_module == nullptr) {
      module_diff->diff_type = ADD;
      module_diff->state_module = nullptr;
      module_diff->active_module = active_module;
    } else {
      damgr_get_diff_module(module_diff, state_module, active_module, arena);
    }
  }

  // pass 2: walk state modules -> REMOVE
  if (state_host != nullptr) {
    for (size_t i = 0; i < state_host->modules_cnt; ++i) {
      Damgr_Module *state_module = state_host->modules[i];
      if (find_module(active_host->modules, active_host->modules_cnt,
                      state_module->module_name) == nullptr) {
        Damgr_Module_Diff *module_diff =
            &host_diff->diffs[host_diff->diff_cnt++];
        module_diff->diff_type = REMOVE;
        module_diff->state_module = state_module;
        module_diff->active_module = nullptr;
      }
    }
  }
}
