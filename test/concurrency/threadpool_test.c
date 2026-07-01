
#define _GNU_SOURCE

#include <cextras/concurrency.h>
#include <pthread.h>
#include <stdatomic.h>
#include <testlib.h>
#include <unistd.h>

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

static unsigned int
ackermann(unsigned int m, unsigned int n) {
	if (m == 0) {
		return n + 1;
	} else if (m > 0 && n == 0) {
		return ackermann(m - 1, 1);
	} else {
		return ackermann(m - 1, ackermann(m, n - 1));
	}
}

static void
thread_func_ackermann(void *arg) {
	unsigned int *store = arg;

	*store = ackermann(2, 6);
}

static void
test_init_cleanup(void) {
	struct CxThreadpool pool = {0};
	int rv = 0;

	rv = cx_threadpool_init(&pool, 2);
	ASSERT_EQ(0, rv);

	rv = cx_threadpool_cleanup(&pool);
	ASSERT_EQ(0, rv);
}

static void
thread_func_inc(void *arg) {
	atomic_uint *counter = arg;
	int rv = 0;

	usleep(1000);
	atomic_fetch_add(counter, 1);
	ASSERT(rv == PTHREAD_BARRIER_SERIAL_THREAD || rv == 0);
}

static void
test_add_task(void) {
	struct CxThreadpool pool = {0};
	int rv = 0;
	atomic_uint counter = 0;

	rv = cx_threadpool_init(&pool, 1);
	ASSERT_EQ(0, rv);

	rv = cx_threadpool_schedule(&pool, thread_func_inc, &counter);
	ASSERT_EQ(0, rv);

	while (atomic_load(&counter) != 1) {
		usleep(1000);
	}

	rv = cx_threadpool_wait(&pool);
	ASSERT_EQ(0, rv);
	rv = cx_threadpool_cleanup(&pool);
	ASSERT_EQ(0, rv);
}

static void
test_add_multiple_tasks_ackermann(void) {
	struct CxThreadpool pool = {0};
	int rv = 0;
	unsigned int ackermann_results[100] = {0};

	rv = cx_threadpool_init(&pool, 0);
	ASSERT_EQ(0, rv);

	for (size_t i = 0; i < LENGTH(ackermann_results); i++) {
		ackermann_results[i] = i;
		rv = cx_threadpool_schedule(
				&pool, thread_func_ackermann, &ackermann_results[i]);
		ASSERT_EQ(0, rv);
	}

	rv = cx_threadpool_wait(&pool);
	ASSERT_EQ(0, rv);
	rv = cx_threadpool_cleanup(&pool);
	ASSERT_EQ(0, rv);

	unsigned int expected = ackermann(2, 6);
	for (size_t i = 0; i < LENGTH(ackermann_results); i++) {
		ASSERT_EQ(expected, ackermann_results[i]);
	}
}

static void
test_add_multiple_tasks(void) {
	struct CxThreadpool pool = {0};
	int rv = 0;
	atomic_int counter = 0;

	rv = cx_threadpool_init(&pool, 0);
	ASSERT_EQ(0, rv);

	for (size_t i = 0; i < 10000; i++) {
		rv = cx_threadpool_schedule(&pool, thread_func_inc, &counter);
		ASSERT_EQ(0, rv);
	}

	rv = cx_threadpool_wait(&pool);
	ASSERT_EQ(0, rv);

	rv = cx_threadpool_cleanup(&pool);
	ASSERT_EQ(0, rv);

	int total = atomic_load(&counter);
	ASSERT_EQ(10000, total);
}

DECLARE_TESTS
TEST(test_init_cleanup)
TEST(test_add_task)
TEST(test_add_multiple_tasks)
TEST(test_add_multiple_tasks_ackermann)
END_TESTS
