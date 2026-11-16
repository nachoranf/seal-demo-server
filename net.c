#include "net.h"
#include "log.h"

#include <unistd.h>

int connect_once(const char *host, int port) {
    (void)host;
    (void)port;
    /* Platform-specific; stands in for a socket connect. */
    return -1;
}

int connect_with_retry(const char *host, int port) {
    int fd = connect_once(host, port);
    if (fd < 0) {
        log_message(LOG_WARN, "connection refused");
    }
    return fd;
}

long send_all(int fd, const char *data, size_t len) {
    ssize_t written = write(fd, data, len);
    if (written < 0) {
        return -1;
    }
    return (long)written;
}
