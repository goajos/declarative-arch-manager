#ifndef DAMGR_CONTEXT_H
#define DAMGR_CONTEXT_H
#include <ctype.h>
#include <string.h>

#include "damgr/path.h"
#include "damgr/result.h"
#include "damgr/utils.h"

constexpr size_t DAMGR_LINE_MAX = 512;
constexpr size_t DAMGR_ARRAY_BUFFER_MAX = 256;

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
  Damgr_Mod **mods;
  size_t mods_cnt;
} Damgr_Host;

typedef struct context {
  const char *aur_helper;
  Damgr_Host *active_host;
  Damgr_Host *state_host;
} Damgr_Context;

Result_t damgr_read_context_result(const Damgr_Paths *paths,
                                   Damgr_Context *context, Damgr_Arena *arena);

#ifdef DAMGR_CONTEXT_IMPLEMENTATION

typedef struct {
  const char *name;
  bool flag;
} Mod_Item;

typedef struct {
  size_t start;
  size_t cnt;
} Mod_Span;

typedef enum { LINE_SKIP, LINE_HEADER, LINE_ITEM } Line_Kind;

typedef struct line {
  Line_Kind kind;
  char *key; // LINE_HEADER only
  char *val; // LINE_HEADER: value after '=', LINE_ITEM: item text
} Line;

constexpr size_t CONF_KEY_CNT = 2;
typedef enum { AUR_HELPER, HOST } Conf_Key;
static const char *const CONF_KEYS[CONF_KEY_CNT] = {[AUR_HELPER] = "aur_helper",
                                                    [HOST] = "host"};

constexpr size_t HOST_KEY_CNT = 1;
typedef enum { MODS } Host_Key;
static const char *const HOST_KEYS[HOST_KEY_CNT] = {[MODS] = "modules"};

constexpr size_t MOD_KEY_CNT = 4;
typedef enum { NONE, DOT, SVC, AUR, PKG } Mod_Key;
static const char *const MOD_KEYS[MOD_KEY_CNT] = {[DOT - 1] = "dotfiles",
                                                  [SVC - 1] = "services",
                                                  [AUR - 1] = "aur_packages",
                                                  [PKG - 1] = "packages"};

typedef enum { NAME_DOT, NAME_PATH, NAME_PKG, NAME_UNIT } Name_Kind;
typedef struct {
  Name_Kind kind;
  bool allow_flag;
} Item_Opts;
static const Item_Opts MOD_OPTS[MOD_KEY_CNT] = {[DOT - 1] = {NAME_DOT, true},
                                                [SVC - 1] = {NAME_UNIT, true},
                                                [AUR - 1] = {NAME_PKG, false},
                                                [PKG - 1] = {NAME_PKG, false}};

static Result_t is_state_dir_empty_result(const char *fid_buffer, bool *empty) {
  DIR *dir;
  DAMGR_CALL(damgr_opendir_result(fid_buffer, &dir));

  int count = 0;
  struct dirent *ent;
  // dot entries ., .., .tmp, .journal are not state files
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

static void strip_comment_inplace(char *str) {
  char *hash = strchr(str, '#');
  if (hash != nullptr)
    *hash = '\0';
}

static Result_t check_line_len_result(const char *line, FILE *fid,
                                      const char *src) {
  if (strchr(line, '\n') == nullptr && !feof(fid))
    return Error("line exceeds %zu chars in %s", DAMGR_LINE_MAX - 2, src);

  return Success;
}

static Result_t classify_line_result(char *line, FILE *fid, const char *src,
                                     Line *out) {
  DAMGR_CALL(check_line_len_result(line, fid, src));
  bool indented = isspace((unsigned char)line[0]);

  strip_comment_inplace(line);
  trim_string_inplace(line);
  if (line[0] == '\0') { // blank, comment, indented comment
    *out = (Line){.kind = LINE_SKIP};
    return Success;
  }

  if (indented) {
    *out = (Line){.kind = LINE_ITEM, .val = line};
    return Success;
  }

  char *eq = strchr(line, '=');
  if (eq == nullptr)
    return Error("expected key=value in %s: %s", src, line);
  *eq = '\0';
  trim_string_inplace(line);
  trim_string_inplace(eq + 1);
  if (line[0] == '\0')
    return Error("empty key in %s", src);

  *out = (Line){.kind = LINE_HEADER, .key = line, .val = eq + 1};
  return Success;
}

static Result_t match_header_result(const char *key, const char *const *keys,
                                    size_t n, bool *seen, size_t *idx,
                                    const char *src) {
  for (size_t i = 0; i < n; ++i) {
    if (strcmp(key, keys[i]) != 0)
      continue;
    if (seen[i]) // no duplicate headers allowed
      return Error("duplicate header in %s: %s", src, key);
    seen[i] = true;
    *idx = i;
    return Success;
  }
  return Error("invalid header in %s: %s", src, key);
}

static bool name_char_ok(Name_Kind kind, unsigned char c) {
  if (isalnum(c))
    return true;
  switch (kind) {
  case NAME_DOT:
  case NAME_PATH:
  case NAME_PKG:
    return strchr("._-@+", c) != nullptr;
  case NAME_UNIT:
    return strchr("._-@\\", c) != nullptr;
  }
  return false;
}

static Result_t validate_name_result(const char *val, const char *src,
                                     Name_Kind kind) {

  if (val[0] == '\0')
    return Error("empty val in %s", src);
  if (val[0] == '-')
    return Error("val may not start with \"-\" in %s: %s", src, val);
  if (kind != NAME_DOT && val[0] == '.')
    return Error("val may not start with \".\" in %s: %s", src, val);
  if (kind == NAME_DOT && (strcmp(val, ".") == 0 || strcmp(val, "..") == 0))
    return Error("val may not be \".\" or \"..\" in %s: %s", src, val);
  for (const char *p = val; *p; ++p)
    if (!name_char_ok(kind, (unsigned char)*p))
      return Error("invalid char '%c' in %s: %s", *p, src, val);
  return Success;
}

static Result_t parse_item_result(Damgr_Arena *arena, char *val, Item_Opts opts,
                                  const char *src, Mod_Item *out) {
  bool flag = false;
  char *sep = strchr(val, ':');
  if (sep != nullptr) {
    if (!opts.allow_flag)
      return Error("unexpected \":\" flag in %s: %s", src, val);

    *sep = '\0';
    char *nval = sep + 1;
    trim_string_inplace(val);
    trim_string_inplace(nval);
    if (strcmp(nval, "true") == 0)
      flag = true;
    else if (strcmp(nval, "false") == 0)
      flag = false;
    else
      return Error("expected true/false flag after \":\" in %s, got \"%s\"",
                   src, nval);
  }
  DAMGR_CALL(validate_name_result(val, src, opts.kind));

  char *dup;
  DAMGR_CALL(damgr_arena_strdup_result(arena, val, &dup));
  out->name = dup;
  out->flag = flag;
  return Success;
}

static Result_t push_item_result(Damgr_Arena *arena, Mod_Item *items,
                                 size_t *cnt, char *val, Item_Opts opts,
                                 const char *src) {
  if (*cnt >= DAMGR_ARRAY_BUFFER_MAX)
    return Error("more than %zu items in %s", DAMGR_ARRAY_BUFFER_MAX, src);
  DAMGR_CALL(parse_item_result(arena, val, opts, src, &items[*cnt]));
  (*cnt)++;
  return Success;
}

static Result_t push_mod_item_result(Damgr_Arena *arena, Mod_Item *items,
                                     size_t *cnt, Mod_Span *span, Mod_Key key,
                                     char *text, const char *src) {
  DAMGR_CALL(push_item_result(arena, items, cnt, text, MOD_OPTS[key - 1], src));
  span[key - 1].cnt++;
  return Success;
}

static Result_t validate_config_result(const Damgr_Context *context,
                                       bool is_state) {
  const char *src = is_state ? "state config" : "active config";
  const Damgr_Host *host =
      is_state ? context->state_host : context->active_host;

  if (host == nullptr)
    return Error("no host in %s", src);
  if (strcmp(host->host_name, "config") == 0)
    return Error("host name \"%s\" in %s clashes with config state file name",
                 host->host_name, src);

  if (!is_state) {
    if (context->aur_helper == nullptr)
      return Error("no aur_helper in %s", src);
  }

  return Success;
}

static Result_t parse_config_result(Damgr_Context *context, Damgr_Arena *arena,
                                    FILE *config_fid, const char *src,
                                    bool is_state) {
  bool seen[CONF_KEY_CNT] = {0};
  char *dup;

  char line[DAMGR_LINE_MAX];
  while (fgets(line, sizeof(line), config_fid)) {
    Line l;
    DAMGR_CALL(classify_line_result(line, config_fid, src, &l));

    switch (l.kind) {
    case LINE_SKIP:
      break;
    case LINE_ITEM:
      return Error("unexpected indented line in %s: %s", src, l.val);
    case LINE_HEADER: {
      size_t idx;
      DAMGR_CALL(
          match_header_result(l.key, CONF_KEYS, CONF_KEY_CNT, seen, &idx, src));

      switch ((Conf_Key)idx) {
      case AUR_HELPER:
        if (!is_state) { // the aur helper of the state config is not used
          DAMGR_CALL(validate_name_result(l.val, src, NAME_PKG));
          DAMGR_CALL(damgr_arena_strdup_result(arena, l.val, &dup));
          context->aur_helper = dup;
        }
        break;
      case HOST: {
        Damgr_Host *host;
        DAMGR_CALL(validate_name_result(l.val, src, NAME_PATH));
        DAMGR_CALL(damgr_arena_alloc_result(arena, sizeof(Damgr_Host),
                                            (void **)&host));
        DAMGR_CALL(damgr_arena_strdup_result(arena, l.val, &dup));
        host->host_name = dup;
        if (is_state)
          context->state_host = host;
        else
          context->active_host = host;
        break;
      }
      }
      break;
    }
    }
  }

  if (ferror(config_fid))
    return Error("failed to read %s", src);
  return Success;
}

static Result_t read_config_result(Damgr_Context *context, Damgr_Arena *arena,
                                   const char *config_path, bool is_state) {
  FILE *config_fid;
  DAMGR_CALL(damgr_fopen_result(config_path, "r", &config_fid));
  auto r =
      parse_config_result(context, arena, config_fid, config_path, is_state);
  fclose(config_fid);
  return r;
}

static Result_t validate_host_result(const Damgr_Host *host) {
  if (host->mods_cnt == 0)
    return Error("host %s has no modules", host->host_name);

  for (size_t i = 0; i < host->mods_cnt; ++i) {
    const char *name = host->mods[i]->mod_name;
    if (strcmp(name, "config") == 0)
      return Error("module name \"%s\" clashes with config state file name",
                   name);
    if (strcmp(name, host->host_name) == 0)
      return Error("module name \"%s\" clashes with host state file name",
                   name);
    for (size_t j = i + 1; j < host->mods_cnt; ++j) {
      if (strcmp(name, host->mods[j]->mod_name) == 0)
        return Error("duplicate module name \"%s\" in host %s", name,
                     host->host_name);
    }
  }

  return Success;
}

static Result_t parse_host_result(Damgr_Host *host, Damgr_Arena *arena,
                                  FILE *host_fid, const char *src) {
  bool seen[HOST_KEY_CNT] = {0};
  size_t cnt = 0;
  bool in_section = false;
  Mod_Item items[DAMGR_ARRAY_BUFFER_MAX];
  Item_Opts opts = {NAME_PATH, false};

  char line[DAMGR_LINE_MAX];
  while (fgets(line, sizeof(line), host_fid)) {
    Line l;
    DAMGR_CALL(classify_line_result(line, host_fid, src, &l));

    switch (l.kind) {
    case LINE_SKIP:
      break;
    case LINE_HEADER: {
      size_t idx;
      DAMGR_CALL(
          match_header_result(l.key, HOST_KEYS, HOST_KEY_CNT, seen, &idx, src));
      in_section = true;
      if (l.val[0] != '\0')
        DAMGR_CALL(push_item_result(arena, items, &cnt, l.val, opts, src));
      break;
    }
    case LINE_ITEM:
      if (!in_section)
        return Error("item before any header in %s: %s", src, l.val);
      char *eq = strchr(l.val, '=');
      if (eq != nullptr)
        return Error("indented header in %s: %s", src, l.val);

      DAMGR_CALL(push_item_result(arena, items, &cnt, l.val, opts, src));
      break;
    }
  }

  if (ferror(host_fid))
    return Error("failed to read %s", src);

  if (cnt == 0)
    return Success; // validate_host_result reports this

  DAMGR_CALL(damgr_arena_alloc_result(arena, cnt * sizeof(Damgr_Mod *),
                                      (void **)&host->mods));
  host->mods_cnt = cnt;
  for (size_t i = 0; i < cnt; ++i) {
    DAMGR_CALL(damgr_arena_alloc_result(arena, sizeof(Damgr_Mod),
                                        (void **)&host->mods[i]));
    host->mods[i]->mod_name = items[i].name;
  }
  return Success;
}

static Result_t read_host_result(Damgr_Host *host, Damgr_Arena *arena,
                                 const char *host_path) {
  FILE *host_fid;
  DAMGR_CALL(damgr_fopen_result(host_path, "r", &host_fid));
  auto r = parse_host_result(host, arena, host_fid, host_path);
  fclose(host_fid);
  return r;
}

static Result_t validate_module_result(const Damgr_Mod *mod, const char *path) {
  if (mod->aur_cnt == 0 && mod->pkg_cnt == 0 && mod->svc_cnt == 0 &&
      mod->dotfile == nullptr)
    return Error("empty module in %s: %s", path, mod->mod_name);

  return Success;
}

static Result_t parse_module_result(Damgr_Mod *module, Damgr_Arena *arena,
                                    FILE *module_fid, const char *src) {
  Mod_Item items[DAMGR_ARRAY_BUFFER_MAX];
  size_t cnt = 0;
  Mod_Key cur_key = NONE;
  Mod_Span span[MOD_KEY_CNT] = {0};
  bool seen[MOD_KEY_CNT] = {0};

  char line[DAMGR_LINE_MAX];
  while (fgets(line, sizeof(line), module_fid)) {
    Line l;
    DAMGR_CALL(classify_line_result(line, module_fid, src, &l));

    switch (l.kind) {
    case LINE_SKIP:
      break;
    case LINE_HEADER: {
      size_t idx;
      DAMGR_CALL(
          match_header_result(l.key, MOD_KEYS, MOD_KEY_CNT, seen, &idx, src));
      cur_key = (Mod_Key)(idx + 1);
      span[idx].start = cnt;
      if (l.val[0] != '\0')
        DAMGR_CALL(push_mod_item_result(arena, items, &cnt, span, cur_key,
                                        l.val, src));
      break;
    }
    case LINE_ITEM:
      if (cur_key == NONE)
        return Error("item before any header in %s: %s", src, l.val);
      char *eq = strchr(l.val, '=');
      if (eq != nullptr)
        return Error("indented header in %s: %s", src, l.val);

      DAMGR_CALL(
          push_mod_item_result(arena, items, &cnt, span, cur_key, l.val, src));
      break;
    }
  }

  if (ferror(module_fid))
    return Error("failed to read %s", src);

  if (span[DOT - 1].cnt > 1)
    return Error("only one dotfile per module allowed in %s", src);

  for (size_t i = 0; i < MOD_KEY_CNT; ++i) {
    Mod_Key key = (Mod_Key)(i + 1);
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
      const Mod_Item *item = &items[offset + j];
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
  auto r = parse_module_result(module, arena, module_fid, module_path);
  fclose(module_fid);
  return r;
}

Result_t damgr_read_context_result(const Damgr_Paths *paths,
                                   Damgr_Context *context, Damgr_Arena *arena) {
  bool empty;
  DAMGR_CALL(is_state_dir_empty_result(paths->state_dir, &empty));

  char fid_buffer[DAMGR_PATH_BUFFER_MAX];
  if (!empty) { // state dir is not empty, so read
    DAMGR_CALL(read_config_result(context, arena, paths->state_config, true));
    DAMGR_CALL(validate_config_result(context, true));

    DAMGR_CALL(
        damgr_resolve_host_path_result(paths, fid_buffer, sizeof(fid_buffer),
                                       context->state_host->host_name, true));

    DAMGR_CALL(read_host_result(context->state_host, arena, fid_buffer));
    DAMGR_CALL(validate_host_result(context->state_host));

    for (size_t i = 0; i < context->state_host->mods_cnt; ++i) {
      DAMGR_CALL(damgr_resolve_module_path_result(
          paths, fid_buffer, sizeof(fid_buffer),
          context->state_host->mods[i]->mod_name, true));
      DAMGR_CALL(
          read_module_result(context->state_host->mods[i], arena, fid_buffer));
    }
  } // not (state dir is not empty)

  DAMGR_CALL(read_config_result(context, arena, paths->active_config, false));
  DAMGR_CALL(validate_config_result(context, false));

  DAMGR_CALL(
      damgr_resolve_host_path_result(paths, fid_buffer, sizeof(fid_buffer),
                                     context->active_host->host_name, false));
  DAMGR_CALL(read_host_result(context->active_host, arena, fid_buffer));
  DAMGR_CALL(validate_host_result(context->active_host));

  for (size_t i = 0; i < context->active_host->mods_cnt; ++i) {
    DAMGR_CALL(damgr_resolve_module_path_result(
        paths, fid_buffer, sizeof(fid_buffer),
        context->active_host->mods[i]->mod_name, false));
    DAMGR_CALL(
        read_module_result(context->active_host->mods[i], arena, fid_buffer));
    DAMGR_CALL(
        validate_module_result(context->active_host->mods[i], fid_buffer));
  }

  return Success;
}

#endif /* DAMGR_CONTEXT_IMPLEMENTATION */
#endif /* DAMGR_CONTEXT_H */
