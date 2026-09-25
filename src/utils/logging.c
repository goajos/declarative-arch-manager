#include "damgr/logging.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static const char *log_levels[] = {"INFO", "ERROR"};

void LOG(enum log_level level, const char *log, ...) {
  time_t now = time(nullptr);
  struct tm *tm_info = localtime(&now);
  char tstring[10];
  strftime(tstring, sizeof(tstring), "%H:%M:%S", tm_info);

  FILE *stream = (level == LOG_ERROR) ? stderr : stdout;
  fprintf(stream, "[%s] [%s] ", tstring, log_levels[level]);

  va_list args;
  va_start(args, log);
  vfprintf(stream, log, args);
  va_end(args);

  fprintf(stream, "\n");
  fflush(stream);
}
