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

Result_t damgr_read_context_r(const Damgr_Paths *paths, Damgr_Context *context,
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

static Result_t is_state_dir_empty_r(const char *fid_buffer, bool *empty) {
  DIR *dir;
  DAMGR_CALL(opendir_r(fid_buffer, &dir));

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
  *empty = (count == 0);
  return Success;
}

// static Result_t append_stack_array_check_r(Damgr_Arena *arena, size_t cnt,
//                                            const char *src, char **dst) {
//   if (cnt >= DAMGR_ARRAY_BUFFER_MAX)
//     return Error(err_fmt("array buffer limit exceeded!"));
//   CALL(damgr_arena_strdup_r(arena, src, dst));
//   return Success;
// }

static Result_t parse_config_r(Damgr_Context *context, Damgr_Arena *arena,
                               FILE *config_fid, bool is_state) {
  char *dup;
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
      DAMGR_CALL(arena_strdup_r(arena, val, &dup));
      if (is_state) {
        context->aur_helper = nullptr;
      } else {
        // only use the active aur_helper
        context->aur_helper = dup;
      }

      // TODO: do we need to replace active_host with host somewhere else?
    } else if (strcmp(key, "host") == 0) {
      Damgr_Host *host;
      DAMGR_CALL(arena_alloc_r(arena, sizeof(Damgr_Host), (void **)&host));
      DAMGR_CALL(arena_strdup_r(arena, val, &dup));
      if (is_state) {
        context->state_host = host;
        context->state_host->host_name = dup;
      } else {
        context->active_host = host;
        context->active_host->host_name = dup;
      }
    }
  }
  return Success;
}

static Result_t read_config_r(Damgr_Context *context, Damgr_Arena *arena,
                              const char *config, bool is_state) {
  FILE *config_fid;
  DAMGR_CALL(fopen_r(config, "r", &config_fid));
  auto r = parse_config_r(context, arena, config_fid, is_state);
  fclose(config_fid);
  return r;
}

// static Result_t(Nullptr_t, ErrMessage_t)
//     read_host_r(Damgr_Host *host, Damgr_Arena *arena, const char *host_path)
//     {
//   auto fid_r = fopen_r(host_path, "r");
//   if (!fid_r.is_success)
//     return Err(Nullptr_t, ErrMessage_t)(fid_r.error);
//
//   FILE *host_fid = fid_r.value;
//
//   Result_t(Ptr_t, ErrMessage_t) ptr_r;
//   char line[DAMGR_LINE_MAX];
//   char *stack_modules[DAMGR_ARRAY_BUFFER_MAX];
//   size_t modules_cnt = 0;
//   bool parsing = false;
//   while (fgets(line, sizeof(line), host_fid)) {
//     if (line[0] == '\n') // skip newlines
//       continue;
//     if (line[0] == '#') // skip comments
//       continue;
//
//     char *eq = strchr(line, '=');
//     if (!eq) {
//       // non = lines should be indented
//       bool is_indented = isspace((unsigned char)line[0]);
//       if (is_indented && parsing) {
//         trim_string_inplace(line);
//         if (line[0] == '\0')
//           continue;
//         if (line[0] == '#') // also skip indented comments
//           continue;
//
//         ptr_r = append_stack_array_check_r(arena, modules_cnt, line);
//         if (!ptr_r.is_success)
//           goto cleanup;
//         stack_modules[modules_cnt++] = ptr_r.value;
//
//       } else { // not (is_indented && parsing)
//         fclose(host_fid);
//         return Err(Nullptr_t, ErrMessage_t)(err_fmt(
//             "detected a not indented line parsing modules for host %s:\n%s",
//             host->host_name, line));
//       }
//     } else { // not !eq
//       *eq = '\0';
//       char *key = line;
//       char *val = eq + 1;
//       trim_string_inplace(key);
//       trim_string_inplace(val);
//       if (strcmp(key, "modules") == 0) {
//         parsing = true;
//       } else {
//         fclose(host_fid);
//         return Err(Nullptr_t, ErrMessage_t)(
//             err_fmt("detected a invalid key parsing  host %s:\n%s",
//                     host->host_name, key));
//       }
//       if (*val != '\0' && *val != '\n') {
//         ptr_r = append_stack_array_check_r(arena, modules_cnt, val);
//         if (!ptr_r.is_success)
//           goto cleanup;
//         stack_modules[modules_cnt++] = ptr_r.value;
//       }
//     }
//   } // end while
//
//   if (modules_cnt > 0) {
//     ptr_r = arena_alloc_r(arena, modules_cnt * sizeof(Damgr_Module *));
//     if (!ptr_r.is_success)
//       goto cleanup;
//     host->modules = ptr_r.value;
//     host->modules_cnt = modules_cnt;
//     for (size_t i = 0; i < modules_cnt; ++i) {
//       ptr_r = arena_alloc_r(arena, modules_cnt * sizeof(Damgr_Module));
//       if (!ptr_r.is_success)
//         goto cleanup;
//       host->modules[i] = ptr_r.value;
//       host->modules[i]->module_name = stack_modules[i];
//     }
//   }
//   fclose(host_fid);
//   return Success(Nullptr_t, ErrMessage_t)(nullptr);
// cleanup:
//   fclose(host_fid);
//   return Err(Nullptr_t, ErrMessage_t)(ptr_r.error);
// }
//
// static Result_t(Nullptr_t, ErrMessage_t)
//     read_module_r(Damgr_Module *module, Damgr_Arena *arena,
//                   const char *module_path) {
//   auto fid_r = fopen_r(module_path, "r");
//   if (!fid_r.is_success)
//     return Err(Nullptr_t, ErrMessage_t)(fid_r.error);
//
//   FILE *module_fid = fid_r.value;
//
//   Result_t(Ptr_t, ErrMessage_t) ptr_r;
//   // TODO: parse module
//
//   fclose(module_fid);
//   return Success(Nullptr_t, ErrMessage_t)(nullptr);
// cleanup:
//   fclose(module_fid);
//   return Err(Nullptr_t, ErrMessage_t)(ptr_r.error);
// }

/*  read context preconditions:
 *    - .state/damgr folder should exist
 *    - .state/damgr folder should be empty, excluding:
 *      - .journal
 *      - .staging/
 *      - .backup/
 * */
Result_t read_context_r(const Damgr_Paths *paths, Damgr_Context *context,
                        Damgr_Arena *arena, struct stat *st) {
  bool empty;
  DAMGR_CALL(make_dir_r(paths->state_dir, st, &empty));

  DAMGR_CALL(is_state_dir_empty_r(paths->state_dir, &empty));

  char fid_buffer[DAMGR_PATH_BUFFER_MAX];
  if (!empty) { // state dir is not empty, so read
    DAMGR_CALL(read_config_r(context, arena, paths->state_config, true));

    DAMGR_CALL(resolve_host_path(paths, fid_buffer, sizeof(fid_buffer),
                                 context->state_host->host_name, true));

    // CALL(read_host_r(context->state_host, arena, fid_buffer));

    for (size_t i = 0; i < context->state_host->modules_cnt; ++i) {
      DAMGR_CALL(resolve_module_path(
          paths, fid_buffer, sizeof(fid_buffer),
          context->state_host->modules[i]->module_name, true));
      // CALL(read_module_r(context->state_host->modules[i], arena,
      // fid_buffer));
    }
  } // not (state dir is not empty)

  DAMGR_CALL(read_config_r(context, arena, paths->active_config, false));

  DAMGR_CALL(resolve_host_path(paths, fid_buffer, sizeof(fid_buffer),
                               context->active_host->host_name, false));
  // CALL(read_host_r(context->active_host, arena, fid_buffer));

  for (size_t i = 0; i < context->active_host->modules_cnt; ++i) {
    DAMGR_CALL(resolve_module_path(
        paths, fid_buffer, sizeof(fid_buffer),
        context->active_host->modules[i]->module_name, true));
    // CALL(read_module_r(context->active_host->modules[i], arena, fid_buffer));
  }

  return Success;
}

#endif /* DAMGR_CONTEXT_IMPLEMENTATION */
#endif /* DAMGR_CONTEXT_H */
