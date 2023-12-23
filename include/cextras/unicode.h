#include <stdint.h>
#include <sys/types.h>

ssize_t cx_utf8_clen(const uint8_t *str, size_t length);

ssize_t cx_utf8_16len(const uint8_t *str, size_t length);

ssize_t cx_utf8_bidx(const uint8_t *str, size_t length, size_t char_index);
