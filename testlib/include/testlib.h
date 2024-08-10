/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : test
 * @created     : Tuesday Jul 25, 2023 13:23:23 CEST
 */

#ifndef TESTLIB_H
#define TESTLIB_H

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

#define ASSERT(a) \
	if (!(a)) { \
		printf("Assertion failed: %s\n", #a); \
		abort(); \
	}
#define ASSERT_TRUE(a) ASSERT(a)
#define ASSERT_FALSE(a) ASSERT(!(a))
#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_STREQ(a, b) \
	if (strcmp((a), (b))) { \
		printf("Assertion failed: %s != %s\n", #a, #b); \
		printf("Assertion actual: %s != %s\n", a, b); \
		abort(); \
	}
#define ASSERT_STRNEQ(a, b, size) \
	if (!strncmp((a), (b), size)) { \
		printf("Assertion failed: %s == %s\n", #a, #b); \
		printf("Assertion actual: %s == %s\n", a, b); \
		abort(); \
	}
#define ASSERT_GT(a, b) ASSERT((a) > (b))
#define ASSERT_LT(a, b) ASSERT((a) < (b))

#

#ifdef __cplusplus
}
#endif
#endif /* TESTLIB_H */
