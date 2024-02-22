#include "../../include/cextras/unicode.h"
#include <stdbool.h>
extern const uint8_t cx__zero_width_tbl[];
extern const uint8_t cx__zero_width_lookup[];
extern const size_t cx__zero_width_size;
extern const size_t cx__zero_width_radix;

extern const uint8_t cx__double_width_tbl[];
extern const uint8_t cx__double_width_lookup[];
extern const size_t cx__double_width_size;
extern const size_t cx__double_width_radix;

static bool
table_lookup(
		const uint8_t *lookup, const uint8_t *tbl, const size_t size,
		const size_t radix, uint32_t cp) {
	if (cp >= size) {
		return false;
	}

	const size_t radix_mask = (1 << radix) - 1;
	const uint16_t index = lookup[cp >> radix];
	const uint8_t *field = &tbl[index * (1 << radix) / 8];
	const uint8_t byte = field[(cp & radix_mask) / 8];
	const uint8_t bit = byte & (1 << (cp % 8));

	return bit;
}

size_t
cx_cp_width(uint32_t cp) {
	size_t columns = 0;
	columns += table_lookup(
			cx__double_width_lookup, cx__double_width_tbl,
			cx__double_width_size, cx__double_width_radix, cp);
	columns += !table_lookup(
			cx__zero_width_lookup, cx__zero_width_tbl, cx__zero_width_size,
			cx__zero_width_radix, cp);
	return columns;
}

ssize_t
cx_utf8_cols(const uint8_t *str, size_t length) {
	size_t columns = 0;

	for (size_t i = 0, char_size = 0; i < length; i += char_size) {
		char_size = cx_utf8_csize(&str[i], length - i);
		int32_t cp = cx_utf8_cp(&str[i], char_size);
		if (cp < 0) {
			return -1;
		}
		columns += cx_cp_width(cp);
	}

	return columns;
}

ssize_t
cx_utf8_colidx(
		const uint8_t *str, size_t length, size_t column, size_t tab_size) {
	size_t byte_index = 0;

	for (; byte_index < length;) {
		if (str[byte_index] == '\t') {
			column = (column / tab_size + 1) * tab_size;
			byte_index += 1;
			continue;
		}

		size_t char_size = cx_utf8_csize(&str[byte_index], length - byte_index);
		int32_t cp = cx_utf8_cp(&str[byte_index], char_size);
		if (cp < 0) {
			return -1;
		}
		size_t char_width = cx_cp_width(cp);
		if (column < char_width) {
			return byte_index;
		}
		column -= char_width;
		byte_index += char_size;
	}

	return byte_index;
}
