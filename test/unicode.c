/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : testlib
 * @created     : Tuesday Jul 25, 2023 13:56:58 CEST
 */

#include "../../include/cextras/unicode.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/wait.h>
#include <testlib.h>

static void
test_unicode_width(void) {
	size_t columns;
	columns = cx_utf8_width((uint8_t *)"\0", 1);
	assert(columns == 0);
	columns = cx_utf8_width((uint8_t *)"a", 1);
	assert(columns == 1);
	columns = cx_utf8_width((uint8_t *)"ä", 2);
	assert(columns == 1);
	columns = cx_utf8_width((uint8_t *)"😃", 4);
	assert(columns == 2);
}

DECLARE_TESTS
TEST(test_unicode_width)
END_TESTS
