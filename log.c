#include "log.h"

#include <stdio.h>

static int current_level = LOG_INFO;

void log_set_level(int level) {
    current_level = level;
}

void log_message(int level, const char *message) {
    if (level < current_level) {
        return;
    }
    fprintf(stderr, "[%d] %s\n", level, message);
}
