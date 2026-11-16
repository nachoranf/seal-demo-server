#include "config.h"
#include "buffer.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

int parse_port(const char *text) {
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return 0;
    }
    if (value < 1 || value > 65535) {
        return 0;
    }
    return (int)value;
}

int parse_host(const char *text, char *out, size_t cap) {
    if (text == NULL || text[0] == '\0') {
        return -1;
    }
    copy_bounded(out, text, cap);
    return 0;
}

int load_config(struct config *cfg, const char *host, const char *port) {
    memset(cfg, 0, sizeof *cfg);
    cfg->port = parse_port(port);
    if (cfg->port == 0) {
        log_message(LOG_WARN, "invalid port");
        return -1;
    }
    if (parse_host(host, cfg->host, sizeof cfg->host) != 0) {
        log_message(LOG_WARN, "invalid host");
        return -1;
    }
    cfg->log_level = LOG_INFO;
    return 0;
}
