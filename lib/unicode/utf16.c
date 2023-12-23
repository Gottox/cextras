#include "../../include/cextras/unicode.h"
#include <utf16_table.h>
#include <utf8_table.h>

ssize_t
cx_utf8_16len(const uint8_t *str, size_t length) {
	size_t len16 = 0;
	for (size_t i = 0; i < length;) {
		uint8_t char_len = utf8_len_map[str[i]];
		len16 += utf16_len_map[str[i]];
		if (char_len == 0) {
			return -1;
		}
		i += char_len;
	}

	return len16;
}
