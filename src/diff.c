#include "damgr/diff.h"
#include "damgr/log.h"
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

static int damgr_get_diff_array(Damgr_Array_Diff *diff, Damgr_Arena *arena) {
  if (diff->active_cnt > 0) {
    diff->only_in_active = arena_alloc(arena, diff->active_cnt);
    if (diff->only_in_active == nullptr) {
      damgr_log(ERROR, "arena alloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
  }
  if (diff->state_cnt > 0) {
    diff->only_in_state = arena_alloc(arena, diff->state_cnt);
    if (diff->only_in_state == nullptr) {
      damgr_log(ERROR, "arena alloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
  }
  for (size_t i = 0; i < diff->active_cnt; ++i) {
    diff->only_in_active[i] =
        !in_array(diff->state_array, diff->state_cnt, diff->active_array[i]);
  }
  for (size_t i = 0; i < diff->state_cnt; ++i) {
    diff->only_in_state[i] =
        !in_array(diff->active_array, diff->active_cnt, diff->state_array[i]);
  }
  return EXIT_SUCCESS;
}

static int damgr_get_diff_module(Damgr_Module_Diff *module_diff,
                                 Damgr_Module *state_module,
                                 Damgr_Module *active_module,
                                 Damgr_Arena *arena) {
  int ret;
  module_diff->diff_type = CHANGE;
  module_diff->state_module = state_module;
  module_diff->active_module = active_module;
  // TODO: does this cover all to_link cases?
  module_diff->to_link_diff = state_module->to_link != active_module->to_link;

  module_diff->packages.state_array = state_module->packages;
  module_diff->packages.state_cnt = state_module->packages_cnt;
  module_diff->packages.active_array = active_module->packages;
  module_diff->packages.active_cnt = active_module->packages_cnt;
  ret = damgr_get_diff_array(&module_diff->packages, arena);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  module_diff->aur_packages.state_array = state_module->aur_packages;
  module_diff->aur_packages.state_cnt = state_module->aur_packages_cnt;
  module_diff->aur_packages.active_array = active_module->aur_packages;
  module_diff->aur_packages.active_cnt = active_module->aur_packages_cnt;
  ret = damgr_get_diff_array(&module_diff->aur_packages, arena);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  module_diff->root_services.state_array = state_module->root_services;
  module_diff->root_services.state_cnt = state_module->root_services_cnt;
  module_diff->root_services.active_array = active_module->root_services;
  module_diff->root_services.active_cnt = active_module->root_services_cnt;
  ret = damgr_get_diff_array(&module_diff->root_services, arena);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  module_diff->user_services.state_array = state_module->user_services;
  module_diff->user_services.state_cnt = state_module->user_services_cnt;
  module_diff->user_services.active_array = active_module->user_services;
  module_diff->user_services.active_cnt = active_module->user_services_cnt;
  ret = damgr_get_diff_array(&module_diff->user_services, arena);
  if (ret == EXIT_FAILURE)
    return EXIT_FAILURE;

  // downgrade type to SKIP if no changes
  bool any = module_diff->to_link_diff;
  any = any || has_changed(&module_diff->packages) ||
        has_changed(&module_diff->aur_packages) ||
        has_changed(&module_diff->root_services) ||
        has_changed(&module_diff->user_services);
  if (!any)
    module_diff->diff_type = SKIP;

  return EXIT_SUCCESS;
}

int damgr_get_diff_host(Damgr_Host_Diff *host_diff, Damgr_Host *state_host,
                        Damgr_Host *active_host, Damgr_Arena *arena) {
  host_diff->host_changed =
      state_host == nullptr ||
      strcmp(state_host->host_name, active_host->host_name) != 0;

  size_t max_cnt =
      active_host->modules_cnt + (state_host ? state_host->modules_cnt : 0);
  if (max_cnt > 0) {
    host_diff->diffs = arena_alloc(arena, max_cnt * sizeof(Damgr_Module_Diff));
    if (host_diff->diffs == nullptr) {
      damgr_log(ERROR, "arena alloc failed and returned nullptr");
      return EXIT_FAILURE;
    }
  }
  host_diff->diff_cnt = 0;

  // pass 1: walk active modules -> ADD or CHANGE or SKIP
  for (size_t i = 0; i < active_host->modules_cnt; ++i) {
    Damgr_Module *active_module = active_host->modules[i];
    Damgr_Module *state_module =
        (state_host && !host_diff->host_changed)
            ? find_module(state_host->modules, state_host->modules_cnt,
                          active_module->module_name)
            : nullptr;
    Damgr_Module_Diff *module_diff = &host_diff->diffs[host_diff->diff_cnt++];
    if (state_module == nullptr) {
      module_diff->diff_type = ADD;
      module_diff->state_module = nullptr;
      module_diff->active_module = active_module;
    } else {
      int ret = damgr_get_diff_module(module_diff, state_module, active_module,
                                      arena);
      if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;
    }
  }

  // pass 2: walk state modules -> REMOVE
  if (state_host != nullptr) {
    for (size_t i = 0; i < state_host->modules_cnt; ++i) {
      Damgr_Module *state_module = state_host->modules[i];
      if (host_diff->host_changed ||
          find_module(active_host->modules, active_host->modules_cnt,
                      state_module->module_name) == nullptr) {
        Damgr_Module_Diff *module_diff =
            &host_diff->diffs[host_diff->diff_cnt++];
        module_diff->diff_type = REMOVE;
        module_diff->state_module = state_module;
        module_diff->active_module = nullptr;
      }
    }
  }

  return EXIT_SUCCESS;
}

static const char *damgr_diff_type_string(Damgr_Diff_Type type) {
  switch (type) {
  case ADD:
    return "ADD";
  case REMOVE:
    return "REMOVE";
  case CHANGE:
    return "CHANGE";
  case SKIP:
    return "SKIP";
  }
  return "UNKNOWN";
}

static void damgr_print_array_diff(const char *label, Damgr_Array_Diff *diff) {
  for (size_t i = 0; i < diff->active_cnt; ++i) {
    if (diff->only_in_active[i])
      damgr_log(INFO, "\t%s + %s", label, diff->active_array[i]);
  }
  for (size_t i = 0; i < diff->state_cnt; ++i) {
    if (diff->only_in_state[i])
      damgr_log(INFO, "\t%s - %s", label, diff->state_array[i]);
  }
}

static void damgr_print_module_diff(Damgr_Module_Diff *module_diff) {
  Damgr_Module *module = module_diff->active_module != nullptr
                             ? module_diff->active_module
                             : module_diff->state_module;
  const char *sign = module_diff->diff_type == REMOVE ? "-" : "+";
  damgr_log(INFO, "module: %s [%s]", module->module_name,
            damgr_diff_type_string(module_diff->diff_type));

  if (module_diff->diff_type == SKIP) {
    return;
  }

  if (module_diff->diff_type == ADD || module_diff->diff_type == REMOVE) {
    for (size_t i = 0; i < module->packages_cnt; ++i)
      damgr_log(INFO, "\tpackage %s %s", sign, module->packages[i]);
    for (size_t i = 0; i < module->aur_packages_cnt; ++i)
      damgr_log(INFO, "\taur_package %s %s", sign, module->aur_packages[i]);
    for (size_t i = 0; i < module->root_services_cnt; ++i)
      damgr_log(INFO, "\troot_service %s %s", sign, module->root_services[i]);
    for (size_t i = 0; i < module->user_services_cnt; ++i)
      damgr_log(INFO, "\tuser_service %s %s", sign, module->user_services[i]);
    if (module->to_link)
      damgr_log(INFO, "\tdotfiles %s true", sign);
    return;
  }

  damgr_print_array_diff("package", &module_diff->packages);
  damgr_print_array_diff("aur_package", &module_diff->aur_packages);
  damgr_print_array_diff("root_service", &module_diff->root_services);
  damgr_print_array_diff("user_service", &module_diff->user_services);
  if (module_diff->to_link_diff)
    damgr_log(INFO, "\tdotfiles %s",
              module_diff->active_module->to_link ? "+ true" : "- false");
}

void damgr_print_host_diff(Damgr_Host_Diff *host_diff) {
  damgr_log(INFO, "host diff: %zu module(s) affected", host_diff->diff_cnt);
  for (size_t i = 0; i < host_diff->diff_cnt; ++i) {
    damgr_print_module_diff(&host_diff->diffs[i]);
  }
}
