#include "buffer.h"

#include <string.h>

size_t copy_bounded(char *dst, const char *src, size_t cap) {
    size_t n = strlen(src);
    if (cap == 0) {
        return 0;
    }
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
    return n;
}

size_t append(char *dst, size_t cap, size_t used, const char *src) {
    if (used >= cap) {
        return used;
    }
    size_t copied = copy_bounded(dst + used, src, cap - used);
    return used + copied;
}
