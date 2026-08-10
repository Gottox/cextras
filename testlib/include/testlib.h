/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : test
 * @created     : Tuesday Jul 25, 2023 13:23:23 CEST
 */

#ifndef TESTLIB_H
#define TESTLIB_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct TestlibTest {
	void (*func)(void);
	const char *name;
	int enabled;
};

#ifdef __cplusplus
#	define DECLARE_TESTS \
		extern "C" const struct TestlibTest testlib_tests[] = {
#else
#	define DECLARE_TESTS const struct TestlibTest testlib_tests[] = {
#endif

#define TEST(func) {func, #func, 1},
#define NO_TEST(func) {func, #func, 0},

#define END_TESTS \
	{ 0, 0, 0 } \
	} \
	;

#define FAIL(f, ...) \
	((void)printf("%s:%i: " f "\n", __FILE__, __LINE__, __VA_ARGS__), \
	 (void)abort())
#define ASSERT(a) \
	do { \
		if (!(a)) { \
			FAIL("%s", #a); \
		} \
	} while (0)
#define ASSERT_TRUE(a) ASSERT(a)
#define ASSERT_FALSE(a) ASSERT(!(a))
#define ASSERT_POINTER(x) ((void)sizeof((const volatile void *){(x)}))
#define ASSERT_NULL(a) \
	do { \
		ASSERT_POINTER(a); \
		ASSERT_FALSE(a); \
	} while (0)
#define ASSERT_NOT_NULL(a) \
	do { \
		ASSERT_POINTER(a); \
		ASSERT(a); \
	} while (0)
#define ASSERT_STREQS(a, b, s) \
	do { \
		const char *a_ = (a); \
		const char *b_ = (b); \
		size_t s_ = (s); \
		if (strncmp(a_, b_, s_)) { \
			FAIL("%s == %s; %s == %s", #a, #b, a_, b_); \
		} \
	} while (0)
#define ASSERT_STRNEQS(a, b, s) \
	do { \
		const char *a_ = (a); \
		const char *b_ = (b); \
		size_t s_ = (s); \
		if (!strncmp(a_, b_, s_)) { \
			FAIL("%s == %s; %s == %s", #a, #b, a_, b_); \
		} \
	} while (0)
#define ASSERT_STREQ(a, b) \
	do { \
		const char *a_ = (a); \
		const char *b_ = (b); \
		if (strcmp(a_, b_)) { \
			FAIL("%s == %s; %s == %s", #a, #b, a_, b_); \
		} \
	} while (0)
#define ASSERT_STRNEQ(a, b) \
	do { \
		const char *a_ = (a); \
		const char *b_ = (b); \
		if (!strcmp(a_, b_)) { \
			FAIL("%s == %s; %s == %s", #a, #b, a_, b_); \
		} \
	} while (0)
#define ASSERT_MEMEQ(a, b, s) \
	do { \
		const void *a_ = (a); \
		const void *b_ = (b); \
		size_t s_ = (s); \
		if (memcmp(a_, b_, s_)) { \
			FAIL("%s == %s (%zu bytes)", #a, #b, s_); \
		} \
	} while (0)
#define ASSERT_MEMNEQ(a, b, s) \
	do { \
		const void *a_ = (a); \
		const void *b_ = (b); \
		size_t s_ = (s); \
		if (!memcmp(a_, b_, s_)) { \
			FAIL("%s != %s (%zu bytes)", #a, #b, s_); \
		} \
	} while (0)
#define ASSERT_OP(a, op, b) \
	do { \
		__typeof__(a) a_ = (a); \
		__typeof__(b) b_ = (b); \
		if (!(a_ op b_)) { \
			FAIL("%s " #op " %s; %" PRIxPTR " " #op " %" PRIxPTR, #a, #b, \
				 (intptr_t)a_, (intptr_t)b_); \
		} \
	} while (0)
#define ASSERT_EQ(a, b) ASSERT_OP(a, ==, b)
#define ASSERT_NE(a, b) ASSERT_OP(a, !=, b)
#define ASSERT_GT(a, b) ASSERT_OP(a, >, b)
#define ASSERT_LT(a, b) ASSERT_OP(a, <, b)
#define ASSERT_GE(a, b) ASSERT_OP(a, >=, b)
#define ASSERT_LE(a, b) ASSERT_OP(a, <=, b)

#

#ifdef __cplusplus
}
#endif
#endif /* TESTLIB_H */
