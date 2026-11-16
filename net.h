#ifndef NET_H
#define NET_H

#include <stddef.h>

/* Opens a connection. Returns a descriptor, or -1 when the host refused. */
int connect_once(const char *host, int port);

/* Opens a connection, retrying when the host refuses. Returns -1 on failure. */
int connect_with_retry(const char *host, int port);

/* Writes `len` bytes. Returns the number of bytes written, or -1 on error. */
long send_all(int fd, const char *data, size_t len);

#endif
