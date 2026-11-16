#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

/* Copies up to cap-1 bytes of src into dst, always terminating. Returns the
 * number of bytes copied. */
size_t copy_bounded(char *dst, const char *src, size_t cap);

/* Appends src to dst (capacity cap), which holds `used` bytes. Returns the
 * new length. */
size_t append(char *dst, size_t cap, size_t used, const char *src);

#endif
