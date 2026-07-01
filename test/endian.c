/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : testlib
 * @created     : Tuesday Jul 25, 2023 13:56:58 CEST
 */

#include "../../include/cextras/endian.h"
#include <stdint.h>
#include <string.h>
#include <sys/wait.h>
#include <testlib.h>

static void
test_le16(void) {
	union {
		uint16_t i;
		uint8_t c[2];
	} u = {.c = {1, 2}};

	u.i = CX_CPU_2_LE16(u.i);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	ASSERT_MEMEQ(u.c, "\x01\x02", 2);
#else
	ASSERT_MEMEQ(u.c, "\x02\x01", 2);
#endif

	u.i = CX_LE_2_CPU16(u.i);
	ASSERT_MEMEQ(u.c, "\x01\x02", 2);
}

static void
test_be16(void) {
	union {
		uint16_t i;
		uint8_t c[2];
	} u = {.c = {1, 2}};

	u.i = CX_CPU_2_BE16(u.i);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	ASSERT_MEMEQ(u.c, "\x01\x02", 2);
#else
	ASSERT_MEMEQ(u.c, "\x02\x01", 2);
#endif

	u.i = CX_BE_2_CPU16(u.i);
	ASSERT_MEMEQ(u.c, "\x01\x02", 2);
}

static void
test_le32(void) {
	union {
		uint32_t i;
		uint8_t c[4];
	} u = {.c = {1, 2, 3, 4}};

	u.i = CX_CPU_2_LE32(u.i);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	ASSERT_MEMEQ(u.c, "\x01\x02\x03\x04", 4);
#else
	ASSERT_MEMEQ(u.c, "\x04\x03\x02\x01", 4);
#endif

	u.i = CX_LE_2_CPU32(u.i);
	ASSERT_MEMEQ(u.c, "\x01\x02\x03\x04", 4);
}

static void
test_be32(void) {
	union {
		uint32_t i;
		uint8_t c[4];
	} u = {.c = {1, 2, 3, 4}};

	u.i = CX_CPU_2_BE32(u.i);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	ASSERT_MEMEQ(u.c, "\x01\x02\x03\x04", 4);
#else
	ASSERT_MEMEQ(u.c, "\x04\x03\x02\x01", 4);
#endif

	u.i = CX_BE_2_CPU32(u.i);
	ASSERT_MEMEQ(u.c, "\x01\x02\x03\x04", 4);
}

static void
test_le64(void) {
	union {
		uint64_t i;
		uint8_t c[8];
	} u = {.c = {1, 2, 3, 4, 5, 6, 7, 8}};

	u.i = CX_CPU_2_LE64(u.i);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	ASSERT_MEMEQ(u.c, "\x01\x02\x03\x04\x05\x06\x07\x08", 8);
#else
	ASSERT_MEMEQ(u.c, "\x08\x07\x06\x05\x04\x03\x02\x01", 8);
#endif

	u.i = CX_LE_2_CPU64(u.i);
	ASSERT_MEMEQ(u.c, "\x01\x02\x03\x04\x05\x06\x07\x08", 8);
}

static void
test_be64(void) {
	union {
		uint64_t i;
		uint8_t c[8];
	} u = {.c = {1, 2, 3, 4, 5, 6, 7, 8}};

	u.i = CX_CPU_2_BE64(u.i);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	ASSERT_MEMEQ(u.c, "\x01\x02\x03\x04\x05\x06\x07\x08", 8);
#else
	ASSERT_MEMEQ(u.c, "\x08\x07\x06\x05\x04\x03\x02\x01", 8);
#endif

	u.i = CX_BE_2_CPU64(u.i);
	ASSERT_MEMEQ(u.c, "\x01\x02\x03\x04\x05\x06\x07\x08", 8);
}

DECLARE_TESTS
TEST(test_le16)
TEST(test_be16)
TEST(test_le32)
TEST(test_be32)
TEST(test_le64)
TEST(test_be64)
END_TESTS
