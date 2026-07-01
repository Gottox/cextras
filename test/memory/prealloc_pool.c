
#define _GNU_SOURCE

#include <cextras/memory.h>
#include <pthread.h>
#include <testlib.h>
#include <unistd.h>

struct MyStruct {
	char dummy[256];
};

static void
test_simple(void) {
	struct CxPreallocPool pool = {0};

	cx_prealloc_pool_init(&pool, sizeof(struct MyStruct));

	struct MyStruct *element = cx_prealloc_pool_get(&pool);
	ASSERT_NOT_NULL(element);

	cx_prealloc_pool_recycle(&pool, element);

	struct MyStruct *element2 = cx_prealloc_pool_get(&pool);
	ASSERT_EQ(element, element2);

	cx_prealloc_pool_recycle(&pool, element2);

	ASSERT_NOT_NULL(pool.reuse_pool);
	cx_prealloc_pool_cleanup(&pool);
}

DECLARE_TESTS
TEST(test_simple)
END_TESTS
