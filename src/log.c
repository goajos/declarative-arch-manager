#include "damgr/log.h"

static const char *level_string(enum damgr_log_level level) {
  switch (level) {
  case INFO:
    return "INFO";
  case DEBUG:
    return "DEBUG";
  case WARNING:
    return "WARNING";
  case ERROR:
    return "ERROR";
  }
  return "?";
}

static void print_prefix(enum damgr_log_level level, const char *func) {
  fprintf(stderr, "[%s][%s] ", level_string(level), func);
}

void damgr_default_log_handler(enum damgr_log_level level, const char *func,
                               const char *fmt, va_list args) {
  print_prefix(level, func);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
}

void damgr_log_at(enum damgr_log_level level, const char *func, const char *fmt,
                  ...) {
  va_list args;
  va_start(args, fmt);
  damgr_default_log_handler(level, func, fmt, args);
  va_end(args);
}
