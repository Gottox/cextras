#include "../../include/cextras/unicode.h"
#include <stdint.h>
extern const uint8_t utf8_len_map[256];

size_t
cx_utf8_csize(const uint8_t *chr, size_t length) {
	uint8_t size = utf8_len_map[chr[0]];
	if (size > length) {
		return 0;
	}
	return size;
}

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

int32_t
cx_utf8_cp(const uint8_t *chr, size_t length) {
	if (!chr) {
		return 0; // return 0 for null string
	}

	uint8_t char_len = cx_utf8_csize((uint8_t *)chr, length);

	switch (char_len) {
	case 1:
		return chr[0];
	case 2:
		return ((chr[0] & 0x1F) << 6) | (chr[1] & 0x3F);
	case 3:
		return ((chr[0] & 0x0F) << 12) | ((chr[1] & 0x3F) << 6) |
				(chr[2] & 0x3F);
	case 4:
		return ((chr[0] & 0x07) << 18) | ((chr[1] & 0x3F) << 12) |
				((chr[2] & 0x3F) << 6) | (chr[3] & 0x3F);
	default:
		return -1;
	}
}
