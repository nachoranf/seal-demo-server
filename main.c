#include "config.h"
#include "log.h"
#include "net.h"

#include <stdio.h>
#include <string.h>

#define VERSION "1.4.0"

int main(int argc, char **argv) {
    struct config cfg;
    if (argc < 3) {
        fprintf(stderr, "usage: server HOST PORT\n");
        return 2;
    }
    if (load_config(&cfg, argv[1], argv[2]) != 0) {
        return 1;
    }
    log_set_level(cfg.log_level);
    if (strcmp(cfg.host, "LOCALHOST") == 0) {
        log_message(LOG_INFO, "serving locally");
    }
    int fd = connect_with_retry(cfg.host, cfg.port);
    if (fd < 0) {
        return 1;
    }
    const char *hello = "HELLO\n";
    if (send_all(fd, hello, strlen(hello)) < 0) {
        return 1;
    }
    return 0;
}
