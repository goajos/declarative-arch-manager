#include "damgr/utils.h"

static const char *active_dir_type_string(Damgr_Active_Dir_Type type) {
  switch (type) {
  case CONTEXT:
    return ".config/damgr";
  case HOST:
    return ".config/damgr/hosts";
  case MODULE:
    return ".config/damgr/modules";
  case DOTFILE:
    return ".config/damgr/dotfiles";
  case NO_ACTIVE_DIR:
    return "";
  }
  return "";
}

static const char *state_dir_type_string(Damgr_State_Dir_Type type) {
  switch (type) {
  case STATE:
    return ".local/state/damgr";
  case STAGING:
    return ".local/state/damgr/.staging";
  case BACKUP:
    return ".local/state/damgr/.backup";
  case NO_STATE_DIR:
    return "";
  }
  return "";
}

void damgr_format_fidbuf(char *fidbuf, const char *name, const bool is_state,
                         const Damgr_Active_Dir_Type active_dir_type,
                         const Damgr_State_Dir_Type state_dir_type) {
  if (is_state) {
    snprintf(fidbuf, DAMGR_FIDBUF_MAX, "%s/%s/%s_state.conf", getenv("HOME"),
             state_dir_type_string(state_dir_type), name);
  } else {
    snprintf(fidbuf, DAMGR_FIDBUF_MAX, "%s/%s/%s.conf", getenv("HOME"),
             active_dir_type_string(active_dir_type), name);
  }
}

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
