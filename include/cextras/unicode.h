#include <stdint.h>
#include <sys/types.h>

size_t cx_utf8_csize(const uint8_t *chr, size_t length);

int32_t cx_utf8_cp(const uint8_t *chr, size_t length);

ssize_t cx_utf8_clen(const uint8_t *str, size_t length);

ssize_t cx_utf8_16len(const uint8_t *str, size_t length);

ssize_t cx_utf8_bidx(const uint8_t *str, size_t length, size_t char_index);

ssize_t cx_utf8_width(const uint8_t *str, size_t length);
