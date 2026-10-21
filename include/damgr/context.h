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
    damgr_read_context_r(const Damgr_Paths *paths, Damgr_Context *context,
                         Damgr_Arena *arena, struct stat *st);

#ifdef DAMGR_CONTEXT_IMPLEMENTATION

static void trim_string_inplace(char *str) {
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

static Result_t(Bool_t, ErrMessage_t)
    is_state_dir_empty_r(const char *fid_buffer) {
  auto dir_r = opendir_r(fid_buffer);
  if (!dir_r.is_success)
    return Err(Bool_t, ErrMessage_t)(dir_r.error);

  DIR *dir = dir_r.value;
  int count = 0;
  struct dirent *ent;
  // .backup and .staging are not .state/damgr entries
  while ((ent = readdir(dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 ||
        strcmp(ent->d_name, DAMGR_BACKUP) == 0 ||
        strcmp(ent->d_name, DAMGR_STAGING) == 0)
      continue;

    if (strcmp(ent->d_name, DAMGR_JOURNAL) == 0)
      continue; // the journal is not a .state/damgr entry

    if (ent->d_type == DT_REG || ent->d_type == DT_LNK || ent->d_type == DT_DIR)
      count++;
  }

  closedir(dir);
  if (count == 0) {
    return Success(Bool_t, ErrMessage_t)(true);
  } else {
    return Success(Bool_t, ErrMessage_t)(false);
  }
}

static Result_t(Ptr_t, ErrMessage_t)
    append_stack_array_check_r(Damgr_Arena *arena, size_t cnt,
                               const char *src) {
  if (cnt >= DAMGR_ARRAY_BUFFER_MAX)
    return Err(Ptr_t, ErrMessage_t)(err_fmt("array buffer limit exceeded!"));
  auto ptr_r = arena_strdup_r(arena, src);
  if (!ptr_r.is_success)
    return Err(Ptr_t, ErrMessage_t)(ptr_r.error);
  return Success(Ptr_t, ErrMessage_t)(ptr_r.value);
}

static Result_t(Nullptr_t, ErrMessage_t)
    read_config_r(Damgr_Context *context, Damgr_Arena *arena,
                  const char *config, bool is_state) {
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
    trim_string_inplace(key);
    trim_string_inplace(val);

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

static Result_t(Nullptr_t, ErrMessage_t)
    read_host_r(Damgr_Host *host, Damgr_Arena *arena, const char *host_path) {
  auto fid_r = fopen_r(host_path, "r");
  if (!fid_r.is_success)
    return Err(Nullptr_t, ErrMessage_t)(fid_r.error);

  FILE *host_fid = fid_r.value;

  Result_t(Ptr_t, ErrMessage_t) ptr_r;
  char line[DAMGR_LINE_MAX];
  char *stack_modules[DAMGR_ARRAY_BUFFER_MAX];
  size_t modules_cnt = 0;
  bool parsing = false;
  while (fgets(line, sizeof(line), host_fid)) {
    if (line[0] == '\n') // skip newlines
      continue;
    if (line[0] == '#') // skip comments
      continue;

    char *eq = strchr(line, '=');
    if (!eq) {
      // non = lines should be indented
      bool is_indented = isspace((unsigned char)line[0]);
      if (is_indented && parsing) {
        trim_string_inplace(line);
        if (line[0] == '\0')
          continue;
        if (line[0] == '#') // also skip indented comments
          continue;

        ptr_r = append_stack_array_check_r(arena, modules_cnt, line);
        if (!ptr_r.is_success)
          goto cleanup;
        stack_modules[modules_cnt++] = ptr_r.value;

      } else { // not (is_indented && parsing)
        fclose(host_fid);
        return Err(Nullptr_t, ErrMessage_t)(err_fmt(
            "detected a not indented line parsing modules for host %s:\n%s",
            host->host_name, line));
      }
    } else { // not !eq
      *eq = '\0';
      char *key = line;
      char *val = eq + 1;
      trim_string_inplace(key);
      trim_string_inplace(val);
      if (strcmp(key, "modules") == 0) {
        parsing = true;
      } else {
        fclose(host_fid);
        return Err(Nullptr_t, ErrMessage_t)(
            err_fmt("detected a invalid key parsing  host %s:\n%s",
                    host->host_name, key));
      }
      if (*val != '\0' && *val != '\n') {
        ptr_r = append_stack_array_check_r(arena, modules_cnt, line);
        if (!ptr_r.is_success)
          goto cleanup;
        stack_modules[modules_cnt++] = ptr_r.value;
      }
    }
  } // end while

  if (modules_cnt > 0) {
    ptr_r = arena_alloc_r(arena, modules_cnt * sizeof(Damgr_Module *));
    if (!ptr_r.is_success)
      goto cleanup;
    host->modules = ptr_r.value;
    for (size_t i = 0; i < modules_cnt; ++i) {
      ptr_r = arena_alloc_r(arena, modules_cnt * sizeof(Damgr_Module));
      if (!ptr_r.is_success)
        goto cleanup;
      host->modules[i] = ptr_r.value;
      host->modules[i]->module_name = stack_modules[i];
    }
  }
  fclose(host_fid);
  return Success(Nullptr_t, ErrMessage_t)(nullptr);
cleanup:
  fclose(host_fid);
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
    damgr_read_context_r(const Damgr_Paths *paths, Damgr_Context *context,
                         Damgr_Arena *arena, struct stat *st) {
  Result_t(Bool_t, ErrMessage_t) bool_r;
  bool_r = damgr_make_dir_r(paths->state_dir, st);
  if (!bool_r.is_success)
    return Err(Nullptr_t, ErrMessage_t)(bool_r.error);

  bool_r = is_state_dir_empty_r(paths->state_dir);
  if (!bool_r.is_success)
    return Err(Nullptr_t, ErrMessage_t)(bool_r.error);

  char fid_buffer[DAMGR_PATH_BUFFER_MAX];
  Result_t(Nullptr_t, ErrMessage_t) nullptr_r;
  if (!bool_r.value) { // state dir is not empty, so read
    nullptr_r = read_config_r(context, arena, paths->state_config, true);
    if (!nullptr_r.is_success)
      return nullptr_r;

    nullptr_r = damgr_resolve_host_path(paths, fid_buffer, sizeof(fid_buffer),
                                        context->state_host->host_name, true);
    if (!nullptr_r.is_success)
      return nullptr_r;
    nullptr_r = read_host_r(context->state_host, arena, fid_buffer);
    if (!nullptr_r.is_success)
      return nullptr_r;

    for (size_t i = 0; i < context->state_host->modules_cnt; ++i) {
      nullptr_r = damgr_resolve_module_path(
          paths, fid_buffer, sizeof(fid_buffer),
          context->state_host->modules[i]->module_name, true);
      if (!nullptr_r.is_success)
        return nullptr_r;
      // nullptr_r =
      //     read_module_r(context->state_host->modules[i], arena, fid_buffer);
      if (!nullptr_r.is_success)
        return nullptr_r;
    }
  } // not (state dir is not empty)

  nullptr_r = read_config_r(context, arena, paths->active_config, false);
  if (!nullptr_r.is_success)
    return nullptr_r;

  nullptr_r = damgr_resolve_host_path(paths, fid_buffer, sizeof(fid_buffer),
                                      context->active_host->host_name, false);
  if (!nullptr_r.is_success)
    return nullptr_r;
  nullptr_r = read_host_r(context->active_host, arena, fid_buffer);
  if (!nullptr_r.is_success)
    return nullptr_r;

  for (size_t i = 0; i < context->active_host->modules_cnt; ++i) {
    nullptr_r = damgr_resolve_module_path(
        paths, fid_buffer, sizeof(fid_buffer),
        context->active_host->modules[i]->module_name, true);
    if (!nullptr_r.is_success)
      return nullptr_r;
    // nullptr_r =
    //     read_module_r(context->active_host->modules[i], arena, fid_buffer);
    if (!nullptr_r.is_success)
      return nullptr_r;
  }

  return Success(Nullptr_t, ErrMessage_t)(nullptr);
}

#endif /* DAMGR_CONTEXT_IMPLEMENTATION */
#endif /* DAMGR_CONTEXT_H */
