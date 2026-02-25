#ifndef LOGGING_H
#define LOGGING_H

enum log_level { LOG_INFO, LOG_ERROR };

void LOG(enum log_level level, const char *log, ...);

#endif /* LOGGING_H */
