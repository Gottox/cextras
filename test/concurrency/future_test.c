#define _GNU_SOURCE

#include <cextras/concurrency.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <testlib.h>
#include <unistd.h>

static void
test_simple_future(void) {
	int marker = 42;
	cx_future_t future = cx_future_init(NULL);
	ASSERT_NOT_NULL(future);

	cx_future_resolve(future, &marker);

	int *result = cx_future_wait(future);
	ASSERT_EQ(&marker, result);

	cx_future_destroy(future);
}

static void
resolver(void *arg) {
	cx_future_t future = arg;

	int *input = cx_future_get_in_value(future);
	ASSERT_NOT_NULL(input);

	int *output = malloc(sizeof(*output));
	ASSERT_NOT_NULL(output);
	*output = *input;
	cx_future_resolve(future, output);
}

static void
waiter(void *arg) {
	cx_future_t future = arg;

	int *result = cx_future_wait(future);

	ASSERT_EQ(42, *result);
	free(result);
	cx_future_destroy(future);
}

static void
test_first_wait_then_resolve(void) {
	int rv = 0;
	int marker = 42;
	struct CxThreadpool pool = {0};
	rv = cx_threadpool_init(&pool, 1);
	ASSERT_EQ(0, rv);

	cx_future_t future = cx_future_init(&marker);

	cx_threadpool_schedule(&pool, waiter, future);

	usleep(10000);

	resolver(future);

	cx_threadpool_cleanup(&pool);
}

static void
test_first_resolve_then_wait(void) {
	int rv = 0;
	int marker = 42;
	struct CxThreadpool pool = {0};
	rv = cx_threadpool_init(&pool, 1);
	ASSERT_EQ(0, rv);

	cx_future_t future = cx_future_init(&marker);

	cx_threadpool_schedule(&pool, resolver, future);

	usleep(10000);

	waiter(future);

	cx_threadpool_cleanup(&pool);
}

DECLARE_TESTS
TEST(test_simple_future)
TEST(test_first_wait_then_resolve)
TEST(test_first_resolve_then_wait)
END_TESTS
