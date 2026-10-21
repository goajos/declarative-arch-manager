#ifndef DAMGR_CONTEXT_H
#define DAMGR_CONTEXT_H
#include <ctype.h>
#include <string.h>

#ifndef DAMGR_RESULT_H
#include "damgr/result.h"
#endif

#ifndef DAMGR_PATHS_H
#include "damgr/paths.h"
#endif

#ifndef DAMGR_UTILS_H
#include "damgr/utils.h"
#endif

constexpr size_t DAMGR_LINE_MAX = 512;
constexpr size_t DAMGR_ARRAY_BUFFER_MAX = 256;

typedef struct module {
  const char *module_name;
} Damgr_Module;

typedef struct host {
  const char *host_name;
  Damgr_Module **modules;
  size_t modules_cnt;
} Damgr_Host;

typedef struct context {
  const char *aur_helper;
  Damgr_Host *active_host;
  Damgr_Host *state_host;
} Damgr_Context;

Result_t(Nullptr_t, ErrMessage_t)
    damgr_read_context(const Damgr_Paths *paths, Damgr_Context *context,
                       Damgr_Arena *arena, struct stat *st);

#ifdef DAMGR_CONTEXT_IMPLEMENTATION

void damgr_trim_string_inplace(char *str) {
  if (*str == '\0')
    return;

  char *start = str;
  while (isspace((unsigned char)*start)) {
    ++start;
  }
  char *end = str + strlen(str) - 1;
  while (end > start && isspace((unsigned char)*end)) {
    --end;
  }
  *(end + 1) = '\0'; // ensure proper null termination
  if (start != str) {
    memmove(str, start, end - start + 2); // +2 includes the null terminator
  }
}

static Result_t(Nullptr_t, ErrMessage_t)
    read_config(Damgr_Context *context, Damgr_Arena *arena, const char *config,
                bool is_state) {
  auto fid_r = fopen_r(config, "r");
  if (!fid_r.is_success)
    return Err(Nullptr_t, ErrMessage_t)(fid_r.error);

  FILE *config_fid = fid_r.value;

  Result_t(Ptr_t, ErrMessage_t) ptr_r;
  char line[DAMGR_LINE_MAX];
  while (fgets(line, sizeof(line), config_fid)) {
    if (line[0] == '\n') // skip newlines
      continue;
    if (line[0] == '#') // skip comments
      continue;

    // valid config lines are all x=y
    char *eq = strchr(line, '=');
    if (!eq)
      continue;

    *eq = '\0';
    char *key = line;
    char *val = eq + 1;
    damgr_trim_string_inplace(key);
    damgr_trim_string_inplace(val);

    if (strcmp(key, "aur_helper") == 0) {
      ptr_r = arena_strdup_r(arena, val);
      if (!ptr_r.is_success)
        goto cleanup;
      if (is_state) {
        context->aur_helper = nullptr;
      } else {
        // only use the active aur_helper
        context->aur_helper = ptr_r.value;
      }

      // TODO: do we need to replace active_host with host somewhere else?
    } else if (strcmp(key, "host") == 0) {
      ptr_r = arena_alloc_r(arena, sizeof(Damgr_Host));
      if (!ptr_r.is_success)
        goto cleanup;
      if (is_state) {
        context->state_host = ptr_r.value;
      } else {
        context->active_host = ptr_r.value;
      }

      ptr_r = arena_strdup_r(arena, val);
      if (!ptr_r.is_success)
        goto cleanup;
      if (is_state) {
        context->state_host->host_name = ptr_r.value;
      } else {
        context->active_host->host_name = ptr_r.value;
      }
    }
  }

  fclose(config_fid);
  return Success(Nullptr_t, ErrMessage_t)(nullptr);
cleanup:
  fclose(config_fid);
  return Err(Nullptr_t, ErrMessage_t)(ptr_r.error);
}

/*  read context preconditions:
 *    - .state/damgr folder should exist
 *    - .state/damgr folder should be empty, excluding:
 *      - .journal
 *      - .staging/
 *      - .backup/
 * */
Result_t(Nullptr_t, ErrMessage_t)
    damgr_read_context(const Damgr_Paths *paths, Damgr_Context *context,
                       Damgr_Arena *arena, struct stat *st) {
  Result_t(Bool_t, ErrMessage_t) bool_r;
  bool_r = damgr_make_dir(paths->state_dir, st);
  if (!bool_r.is_success) {
    return Err(Nullptr_t, ErrMessage_t)(bool_r.error);
  }

  Result_t(Nullptr_t, ErrMessage_t) nullptr_r;
  if (!damgr_is_dir_empty(paths->state_dir).is_success) {
    nullptr_r = read_config(context, arena, paths->state_config, true);
    if (!nullptr_r.is_success)
      return nullptr_r;
  }

  nullptr_r = read_config(context, arena, paths->active_config, false);
  if (!nullptr_r.is_success)
    return nullptr_r;

  return Success(Nullptr_t, ErrMessage_t)(nullptr);
}

#endif /* DAMGR_CONTEXT_IMPLEMENTATION */
#endif /* DAMGR_CONTEXT_H */
