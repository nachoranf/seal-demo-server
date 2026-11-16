#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

struct config {
    int port;
    char host[64];
    int log_level;
};

/* Parses a decimal port. Returns 0 when the text is not a valid port. */
int parse_port(const char *text);

/* Copies the host into `out` (capacity `cap`). Returns 0 on success. */
int parse_host(const char *text, char *out, size_t cap);

/* Fills `cfg` from host and port text. Returns 0 on success, -1 otherwise. */
int load_config(struct config *cfg, const char *host, const char *port);

#endif
