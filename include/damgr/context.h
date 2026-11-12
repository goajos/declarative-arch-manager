#ifndef DAMGR_CONTEXT_H
#define DAMGR_CONTEXT_H
#include <ctype.h>
#include <string.h>

#include "damgr/path.h"
#include "damgr/result.h"
#include "damgr/utils.h"

constexpr size_t DAMGR_LINE_MAX = 512;
constexpr size_t ARRAY_BUFFER_MAX = 256;
constexpr size_t MOD_KEY_CNT = 4; // excluding NONE

typedef enum module_key { NONE, AUR, PKG, SVC, DOT } Damgr_Mod_Key;

typedef struct module_span {
  size_t start;
  size_t cnt;
} Damgr_Mod_Span;

typedef struct module_item {
  const char *name;
  bool flag;
} Damgr_Mod_Item;

typedef struct module {
  const char *mod_name;
  const char **aur;
  size_t aur_cnt;
  const char **pkg;
  size_t pkg_cnt;
  const char **svc;
  bool *svc_flag;
  size_t svc_cnt;
  const char *dotfile;
  bool to_link;
} Damgr_Mod;

typedef struct host {
  const char *host_name;
  Damgr_Mod **modules;
  size_t modules_cnt;
} Damgr_Host;

typedef struct context {
  char *aur_helper;
  Damgr_Host *active_host;
  Damgr_Host *state_host;
} Damgr_Context;

Result_t damgr_read_context_result(const Damgr_Paths *paths,
                                   Damgr_Context *context, Damgr_Arena *arena);

#ifdef DAMGR_CONTEXT_IMPLEMENTATION

static const char *module_key_string(const Damgr_Mod_Key key) {
  switch (key) {
  case AUR:
    return "aur_packages";
  case PKG:
    return "packages";
  case SVC:
    return "services";
  case DOT:
    return "dotfiles";
  case NONE:
    return "";
  }
  return "";
}

static void trim_string_inplace(char *str) {
  char *start = str;
  while (isspace((unsigned char)*start))
    ++start;

  if (*start == '\0') { // only whitespaces
    *str = '\0';
    return;
  }

  char *end = str + strlen(str) - 1;
  while (end > start && isspace((unsigned char)*end))
    --end;

  size_t len = (size_t)(end - start + 1);
  if (start != str)
    memmove(str, start, len);
  str[len] = '\0';
}

static Result_t check_line_len_result(const char *line, FILE *fid,
                                      const char *src) {
  if (strchr(line, '\n') == nullptr && !feof(fid))
    return Error("line exceeds %zu chars in src file %s", DAMGR_LINE_MAX - 2,
                 src);

  return Success;
}

static Result_t is_state_dir_empty_result(const char *fid_buffer, bool *empty) {
  DIR *dir;
  DAMGR_CALL(damgr_opendir_result(fid_buffer, &dir));

  int count = 0;
  struct dirent *ent;
  // .tmp and are not .state/damgr entries
  while ((ent = readdir(dir)) != nullptr) {
    if (ent->d_name[0] == '.')
      continue;

    if (ent->d_type == DT_REG || ent->d_type == DT_LNK ||
        ent->d_type == DT_DIR || ent->d_type == DT_UNKNOWN)
      count++;
  }

  closedir(dir);
  *empty = (count == 0);
  return Success;
}

static Result_t append_stack_array_check_result(Damgr_Arena *arena, size_t cnt,
                                                const char *src, char **dst) {
  if (cnt >= ARRAY_BUFFER_MAX)
    return Error("array buffer limit exceeded!");
  DAMGR_CALL(damgr_arena_strdup_result(arena, src, dst));
  return Success;
}

static Result_t parse_config_result(Damgr_Context *context, Damgr_Arena *arena,
                                    FILE *config_fid, bool is_state) {
  Damgr_Host *host = {0};
  char *dup;

  const char *is_state_str = is_state ? "state" : "active";
  bool aur_parsed = false;
  bool host_parsed = false;

  char line[DAMGR_LINE_MAX];
  while (fgets(line, sizeof(line), config_fid)) {
    DAMGR_CALL(check_line_len_result(line, config_fid, is_state_str));
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
      if (aur_parsed)
        return Error("detected a duplicate header parsing %s config: %s",
                     is_state_str, key);
      aur_parsed = true;

      if (!is_state)
        DAMGR_CALL(damgr_arena_strdup_result(arena, val, &context->aur_helper));

    } else if (strcmp(key, "host") == 0) {
      if (host_parsed)
        return Error("detected a duplicate header parsing %s config: %s",
                     is_state_str, key);
      host_parsed = true;

      DAMGR_CALL(
          damgr_arena_alloc_result(arena, sizeof(Damgr_Host), (void **)&host));
      DAMGR_CALL(damgr_arena_strdup_result(arena, val, &dup));
      if (is_state) {
        context->state_host = host;
        context->state_host->host_name = dup;
      } else {
        context->active_host = host;
        context->active_host->host_name = dup;
      }
    } else {
      return Error("detected an invalid header parsing %s config: %s",
                   is_state_str, key);
    }
  }
  return Success;
}

static Result_t read_config_result(Damgr_Context *context, Damgr_Arena *arena,
                                   const char *config, bool is_state) {
  FILE *config_fid;
  DAMGR_CALL(damgr_fopen_result(config, "r", &config_fid));
  auto r = parse_config_result(context, arena, config_fid, is_state);
  fclose(config_fid);
  return r;
}

static Result_t validate_host_result(const Damgr_Host *host) {
  if (host->modules_cnt == 0)
    return Error("host %s has no modules", host->host_name);

  for (size_t i = 0; i < host->modules_cnt; ++i) {
    const char *name = host->modules[i]->mod_name;
    if (strcmp(name, "config") == 0)
      return Error("module name \"%s\" clashes with config state file name",
                   name);
    if (strcmp(name, host->host_name) == 0)
      return Error("module name \"%s\" clashes with host state file name",
                   name);
    for (size_t j = i + 1; j < host->modules_cnt; ++j) {
      if (strcmp(name, host->modules[j]->mod_name) == 0)
        return Error("duplicate module name \"%s\" in host %s", name,
                     host->host_name);
    }
  }

  return Success;
}

static Result_t parse_host_result(Damgr_Host *host, Damgr_Arena *arena,
                                  FILE *host_fid) {
  Damgr_Mod **modules = {0};
  Damgr_Mod *module = {0};
  char *stack_modules[ARRAY_BUFFER_MAX] = {0};
  size_t modules_cnt = 0;
  char *dup;

  bool modules_parsed = false;

  char line[DAMGR_LINE_MAX];
  while (fgets(line, sizeof(line), host_fid)) {
    DAMGR_CALL(check_line_len_result(line, host_fid, host->host_name));
    if (line[0] == '\n') // skip newlines
      continue;
    if (line[0] == '#') // skip comments
      continue;

    char *eq = strchr(line, '=');
    if (!eq) {
      // non = lines should be indented
      if (!isspace((unsigned char)line[0]))
        return Error(
            "detected a not indented line parsing modules for host %s: %s",
            host->host_name, line);

      // is indented
      trim_string_inplace(line);
      if (line[0] == '\0')
        continue;
      if (line[0] == '#') // also skip indented comments
        continue;

      if (!modules_parsed)
        return Error("detected an item before parsing modules for host %s: %s",
                     host->host_name, line);

      DAMGR_CALL(
          append_stack_array_check_result(arena, modules_cnt, line, &dup));
      stack_modules[modules_cnt++] = dup;

    } else { // not !eq
      *eq = '\0';
      char *key = line;
      char *val = eq + 1;
      trim_string_inplace(key);
      trim_string_inplace(val);
      if (strcmp(key, "modules") == 0) {
        if (modules_parsed)
          return Error("detected a duplicate header parsing host %s: %s",
                       host->host_name, "modules");
        modules_parsed = true;

        if (*val != '\0' && *val != '\n') {
          DAMGR_CALL(
              append_stack_array_check_result(arena, modules_cnt, val, &dup));
          stack_modules[modules_cnt++] = dup;
        }

      } else {
        return Error("detected an invalid header parsing host %s: %s",
                     host->host_name, key);
      }
    }
  } // end while

  if (modules_cnt > 0) {
    DAMGR_CALL(damgr_arena_alloc_result(
        arena, modules_cnt * sizeof(Damgr_Mod *), (void **)&modules));
    host->modules = modules;
    host->modules_cnt = modules_cnt;
    for (size_t i = 0; i < modules_cnt; ++i) {
      DAMGR_CALL(
          damgr_arena_alloc_result(arena, sizeof(Damgr_Mod), (void **)&module));
      host->modules[i] = module;
      host->modules[i]->mod_name = stack_modules[i];
    }
  }
  return Success;
}

static Result_t read_host_result(Damgr_Host *host, Damgr_Arena *arena,
                                 const char *host_path) {
  FILE *host_fid;
  DAMGR_CALL(damgr_fopen_result(host_path, "r", &host_fid));
  auto r = parse_host_result(host, arena, host_fid);
  fclose(host_fid);
  return r;
}

static Result_t parse_item_result(Damgr_Arena *arena, char *src,
                                  const size_t cnt, Damgr_Mod_Item *out) {
  bool flag = false;
  char *sep = strchr(src, ':');
  if (sep) {
    *sep = '\0';
    char *key = src;
    char *val = sep + 1;
    trim_string_inplace(key);
    trim_string_inplace(val);
    if (strcmp(val, "true") == 0)
      flag = true;
    else if (strcmp(val, "false") == 0)
      flag = false;
    else
      return Error("expected true/false after \":\" got \"%s\"", val);
  } else { // not sep
    trim_string_inplace(src);
  }
  if (*src == '\0')
    return Error("empty item name");
  char *dup;
  DAMGR_CALL(append_stack_array_check_result(arena, cnt, src, &dup));
  out->name = dup;
  out->flag = flag;

  return Success;
}

static Result_t parse_module_result(Damgr_Mod *module, Damgr_Arena *arena,
                                    FILE *module_fid) {
  Damgr_Mod_Item *stack_item = {0};
  Damgr_Mod_Item *stack_items[ARRAY_BUFFER_MAX] = {0};
  size_t cnt = 0;
  Damgr_Mod_Key cur_key = NONE;
  Damgr_Mod_Span span[MOD_KEY_CNT] = {0};
  bool seen[MOD_KEY_CNT] = {0};

  char line[DAMGR_LINE_MAX];
  while (fgets(line, sizeof(line), module_fid)) {
    DAMGR_CALL(check_line_len_result(line, module_fid, module->mod_name));
    if (line[0] == '\n') // skip newlines
      continue;
    if (line[0] == '#') // skip comments
      continue;

    // all first level lines start with  x=y
    char *eq = strchr(line, '=');
    if (!eq) {
      // non = lines should be indented
      if (!isspace((unsigned char)line[0]))
        return Error("detected a not indented line parsing module %s: %s",
                     module->mod_name, line);

      if (cur_key == NONE)
        continue;

      // is indented
      trim_string_inplace(line);
      if (line[0] == '\0')
        continue;
      if (line[0] == '#') // also skip indented comments
        continue;

      DAMGR_CALL(damgr_arena_alloc_result(arena, sizeof(Damgr_Mod_Item),
                                          (void **)&stack_item));
      DAMGR_CALL(parse_item_result(arena, line, cnt, stack_item));
      stack_items[cnt++] = stack_item;
      span[cur_key - 1].cnt++;
    } else { // not !eq
      *eq = '\0';
      char *key = line;
      char *val = eq + 1;
      trim_string_inplace(key);
      trim_string_inplace(val);

      Damgr_Mod_Key next_key;
      if (strcmp(key, "aur_packages") == 0)
        next_key = AUR;
      else if (strcmp(key, "packages") == 0)
        next_key = PKG;
      else if (strcmp(key, "services") == 0)
        next_key = SVC;
      else if (strcmp(key, "dotfiles") == 0)
        next_key = DOT;
      else
        return Error("detected an invalid key parsing module %s: %s",
                     module->mod_name, key);

      // prevent data corruption with duplicate headers
      if (seen[next_key - 1])
        return Error("detected a duplicate header parsing module %s: %s",
                     module->mod_name, module_key_string(next_key));
      seen[next_key - 1] = true;

      span[next_key - 1].start = cnt;
      cur_key = next_key;

      if (*val != '\0' && *val != '\n') {
        DAMGR_CALL(damgr_arena_alloc_result(arena, sizeof(Damgr_Mod_Item),
                                            (void **)&stack_item));
        DAMGR_CALL(parse_item_result(arena, val, cnt, stack_item));
        stack_items[cnt++] = stack_item;
        span[cur_key - 1].cnt++;
      }
    }
  }

  for (size_t i = 0; i < MOD_KEY_CNT; ++i) {
    Damgr_Mod_Key key = (Damgr_Mod_Key)(i + 1);
    size_t offset = span[i].start;
    size_t mod_cnt = span[i].cnt;

    if (mod_cnt == 0)
      continue;

    switch (key) {
    case AUR:
      DAMGR_CALL(damgr_arena_alloc_result(arena, mod_cnt * sizeof(char *),
                                          (void **)&module->aur));
      module->aur_cnt = mod_cnt;
      break;
    case PKG:
      DAMGR_CALL(damgr_arena_alloc_result(arena, mod_cnt * sizeof(char *),
                                          (void **)&module->pkg));
      module->pkg_cnt = mod_cnt;
      break;
    case SVC:
      DAMGR_CALL(damgr_arena_alloc_result(arena, mod_cnt * sizeof(char *),
                                          (void **)&module->svc));
      DAMGR_CALL(damgr_arena_alloc_result(arena, mod_cnt * sizeof(bool),
                                          (void **)&module->svc_flag));
      module->svc_cnt = mod_cnt;
      break;
    case DOT:
    case NONE:
      break;
    }

    for (size_t j = 0; j < mod_cnt; ++j) {
      Damgr_Mod_Item *item = stack_items[offset + j];
      switch (key) {
      case AUR:
        module->aur[j] = item->name;
        break;
      case PKG:
        module->pkg[j] = item->name;
        break;
      case SVC:
        module->svc[j] = item->name;
        module->svc_flag[j] = item->flag;
        break;
      case DOT:
        module->dotfile = item->name;
        module->to_link = item->flag;
        break;
      case NONE:
        break;
      }
    }
  }

  return Success;
}

static Result_t read_module_result(Damgr_Mod *module, Damgr_Arena *arena,
                                   const char *module_path) {
  FILE *module_fid;
  DAMGR_CALL(damgr_fopen_result(module_path, "r", &module_fid));
  auto r = parse_module_result(module, arena, module_fid);
  fclose(module_fid);
  return r;
}

Result_t damgr_read_context_result(const Damgr_Paths *paths,
                                   Damgr_Context *context, Damgr_Arena *arena) {
  bool empty;
  DAMGR_CALL(is_state_dir_empty_result(paths->state_dir, &empty));

  char fid_buffer[PATH_BUFFER_MAX];
  if (!empty) { // state dir is not empty, so read
    DAMGR_CALL(read_config_result(context, arena, paths->state_config, true));
    if (context->state_host == nullptr)
      return Error("failed to parse state host");

    DAMGR_CALL(
        damgr_resolve_host_path_result(paths, fid_buffer, sizeof(fid_buffer),
                                       context->state_host->host_name, true));

    DAMGR_CALL(read_host_result(context->state_host, arena, fid_buffer));
    DAMGR_CALL(validate_host_result(context->state_host));

    for (size_t i = 0; i < context->state_host->modules_cnt; ++i) {
      DAMGR_CALL(damgr_resolve_module_path_result(
          paths, fid_buffer, sizeof(fid_buffer),
          context->state_host->modules[i]->mod_name, true));
      DAMGR_CALL(read_module_result(context->state_host->modules[i], arena,
                                    fid_buffer));
    }
  } // not (state dir is not empty)

  DAMGR_CALL(read_config_result(context, arena, paths->active_config, false));
  if (context->active_host == nullptr)
    return Error("failed to parse active host");
  if (context->aur_helper == nullptr || *context->aur_helper == '\0')
    return Error("failed to parse aur_helper from active config");

  DAMGR_CALL(
      damgr_resolve_host_path_result(paths, fid_buffer, sizeof(fid_buffer),
                                     context->active_host->host_name, false));
  DAMGR_CALL(read_host_result(context->active_host, arena, fid_buffer));
  DAMGR_CALL(validate_host_result(context->active_host));

  for (size_t i = 0; i < context->active_host->modules_cnt; ++i) {
    DAMGR_CALL(damgr_resolve_module_path_result(
        paths, fid_buffer, sizeof(fid_buffer),
        context->active_host->modules[i]->mod_name, false));
    DAMGR_CALL(read_module_result(context->active_host->modules[i], arena,
                                  fid_buffer));
  }

  return Success;
}

#endif /* DAMGR_CONTEXT_IMPLEMENTATION */
#endif /* DAMGR_CONTEXT_H */
