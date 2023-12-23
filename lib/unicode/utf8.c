#include "../../include/cextras/unicode.h"
#include <utf16_table.h>
#include <utf8_table.h>

ssize_t
cx_utf8_clen(const uint8_t *str, size_t length) {
	size_t count = 0;
	for (size_t i = 0; i < length; count++) {
		uint8_t char_len = utf8_len_map[str[i]];
		if (char_len == 0) {
			return -1;
		}
		i += char_len;
	}
	return count;
}

ssize_t
cx_utf8_bidx(const uint8_t *str, size_t length, size_t char_index) {
	size_t byte_index = 0;
	for (size_t i = 0; byte_index < length && i < char_index; i++) {
		uint8_t char_len = utf8_len_map[str[byte_index]];
		if (char_len == 0) {
			return -1;
		}
		byte_index += char_len;
	}
	if (byte_index > length) {
		return -1;
	}
	return byte_index;
}
