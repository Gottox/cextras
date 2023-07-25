/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : test
 * @created     : Tuesday Jul 25, 2023 13:23:23 CEST
 */

#ifndef TESTLIB_H

#define TESTLIB_H

struct TestlibTest {
	void (*func)(void);
	const char *name;
	_Bool enabled;
};

#define DECLARE_TESTS const struct TestlibTest testlib_tests[] = {
#define TEST(func) {func, #func, 1},
#define NO_TEST(func) {func, #func, 0},

#define END_TESTS \
	{ 0 } \
	} \
	;

#endif /* TESTLIB_H */
