#include "damgr/log.h"

static damgr_log_handler *damgr__log_handler = &damgr_default_log_handler;

void damgr_default_log_handler(enum damgr_log_level level, const char *fmt,
                               va_list args) {
  switch (level) {
  case INFO:
    fprintf(stderr, "[INFO] ");
    break;
  case WARNING:
    fprintf(stderr, "[WARNING] ");
    break;
  case ERROR:
    fprintf(stderr, "[ERROR] ");
    break;
  }
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
}

void damgr_log(enum damgr_log_level level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  damgr__log_handler(level, fmt, args);
  va_end(args);
}
