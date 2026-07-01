
#define _GNU_SOURCE

#include <cextras/memory.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <testlib.h>
#include <unistd.h>

struct MyStruct {
	struct CxRc rc;
};

static void
test_simple(void) {
	bool should_free;
	struct MyStruct s = {0};
	cx_rc_init(&s.rc);
	ASSERT_EQ(1, s.rc.count);
	cx_rc_retain(&s.rc);
	ASSERT_EQ(2, s.rc.count);
	should_free = cx_rc_release(&s.rc);
	ASSERT_FALSE(should_free);
	ASSERT_EQ(1, s.rc.count);
	should_free = cx_rc_release(&s.rc);
	ASSERT_TRUE(should_free);
	ASSERT_EQ(0, s.rc.count);
}

DECLARE_TESTS
TEST(test_simple)
END_TESTS
