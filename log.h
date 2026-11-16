#ifndef LOG_H
#define LOG_H

enum { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARN = 2 };

void log_set_level(int level);
void log_message(int level, const char *message);

#endif
