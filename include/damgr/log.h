#ifndef DAMGR_LOG_H
#define DAMGR_LOG_H

#include <stdarg.h>
#include <stdio.h>

enum damgr_log_level { INFO, DEBUG, WARNING, ERROR };

// function pointer
typedef void(damgr_log_handler)(enum damgr_log_level level, const char *func,
                                const char *fmt, va_list args);
damgr_log_handler damgr_default_log_handler;

void damgr_log_at(enum damgr_log_level level, const char *func, const char *fmt,
                  ...);
#define damgr_log(level, ...) damgr_log_at(level, __func__, __VA_ARGS__)

#endif /* DAMGR_LOG_H */
