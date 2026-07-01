/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : testlib
 * @created     : Tuesday Jul 25, 2023 13:56:58 CEST
 */

#include "../../include/cextras/unicode.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/wait.h>
#include <testlib.h>

static void
test_unicode_width(void) {
	size_t columns;
	columns = cx_utf8_cols((uint8_t *)"\0", 1);
	ASSERT_EQ(0, columns);
	columns = cx_utf8_cols((uint8_t *)"a", 1);
	ASSERT_EQ(1, columns);
	columns = cx_utf8_cols((uint8_t *)"ä", 2);
	ASSERT_EQ(1, columns);
	columns = cx_utf8_cols((uint8_t *)"😃", 4);
	ASSERT_EQ(2, columns);
}

static void
test_unicode_colidx(void) {
	size_t idx;

	idx = cx_utf8_colidx((uint8_t *)"a😃", 5, 3, 8);
	ASSERT_EQ(5, idx);
}

static void
test_unicode_colidx_tab(void) {
	size_t idx;

	idx = cx_utf8_colidx((uint8_t *)"a\t", 2, 3, 8);
	ASSERT_EQ(2, idx);
}

DECLARE_TESTS
TEST(test_unicode_width)
TEST(test_unicode_colidx)
TEST(test_unicode_colidx_tab)
END_TESTS
