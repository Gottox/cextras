/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : testlib
 */

#include <testlib.h>
#include "../../include/cextras/macro.h"

static void
array_size(void) {
	uint64_t arr[1024] = {0};
	ASSERT_EQ(CX_LENGTH(arr), 1024);
}

DECLARE_TESTS
TEST(array_size)
END_TESTS
