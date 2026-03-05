#ifndef DAMGR_LOG_H
#define DAMGR_LOG_H

#include <stdarg.h>
#include <stdio.h>

enum damgr_log_level { INFO, WARNING, ERROR };

// function pointer
typedef void(damgr_log_handler)(enum damgr_log_level level, const char *fmt,
                                va_list args);

damgr_log_handler damgr_default_log_handler;

void damgr_log(enum damgr_log_level level, const char *fmt, ...);

#endif /* DAMGR_LOG_H */
