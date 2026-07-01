/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2023, Enno Boland
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @author       Enno Boland (mail@eboland.de)
 * @file         vec_test.c
 */

#include <cextras/collection.h>
#include <stdint.h>
#include <string.h>
#include <testlib.h>

static void
init_vec(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int));
	ASSERT_EQ(0, rv);
	ASSERT_EQ(0, vec.size);
	ASSERT_EQ(0, vec.capacity);
	ASSERT_EQ(sizeof(int), vec.element_size);
	ASSERT_NULL(vec.data);

	cx_vec_cleanup(&vec);
	ASSERT_NULL(vec.data);
	ASSERT_EQ(0, vec.size);
	ASSERT_EQ(0, vec.capacity);
	ASSERT_EQ(0, vec.element_size);
}

static void
reserve_capacity(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(uint64_t));
	ASSERT_EQ(0, rv);
	ASSERT_EQ(0, vec.capacity);
	ASSERT_NULL(vec.data);

	rv = cx_vec_reserve(&vec, 4);
	ASSERT_EQ(0, rv);
	ASSERT_EQ(4, vec.capacity);
	ASSERT_NOT_NULL(vec.data);
	ASSERT_EQ(sizeof(uint64_t), vec.element_size);

	/* Reserving a smaller capacity is a no-op. */
	rv = cx_vec_reserve(&vec, 2);
	ASSERT_EQ(0, rv);
	ASSERT_EQ(4, vec.capacity);

	cx_vec_cleanup(&vec);
}

static void
push_and_get(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int));
	ASSERT_EQ(0, rv);
	rv = cx_vec_reserve(&vec, 4);
	ASSERT_EQ(0, rv);

	for (int i = 0; i < 4; i++) {
		int v = i * 10;
		void *p = cx_vec_push(&vec, &v);
		ASSERT_NOT_NULL(p);
		ASSERT_EQ(v, *(int *)p);
	}
	ASSERT_EQ(4, vec.size);
	ASSERT_EQ(4, vec.capacity);

	for (int i = 0; i < 4; i++) {
		int *p = cx_vec_get(&vec, (size_t)i);
		ASSERT_NOT_NULL(p);
		ASSERT_EQ(i * 10, *p);
	}

	cx_vec_cleanup(&vec);
}

static void
get_out_of_bounds(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int));
	ASSERT_EQ(0, rv);

	ASSERT_NULL(cx_vec_get(&vec, 0));

	int v = 42;
	void *p = cx_vec_push(&vec, &v);
	ASSERT_NOT_NULL(p);

	ASSERT_NOT_NULL(cx_vec_get(&vec, 0));
	ASSERT_NULL(cx_vec_get(&vec, 1));
	ASSERT_NULL(cx_vec_get(&vec, 100));

	cx_vec_cleanup(&vec);
}

static void
push_grows_capacity(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int));
	ASSERT_EQ(0, rv);
	rv = cx_vec_reserve(&vec, 2);
	ASSERT_EQ(0, rv);
	ASSERT_EQ(2, vec.capacity);

	int a = 1, b = 2, c = 3;
	void *p = cx_vec_push(&vec, &a);
	ASSERT_NOT_NULL(p);
	p = cx_vec_push(&vec, &b);
	ASSERT_NOT_NULL(p);
	ASSERT_EQ(2, vec.capacity);

	p = cx_vec_push(&vec, &c);
	ASSERT_NOT_NULL(p);
	ASSERT_EQ(3, vec.size);
	ASSERT_LE(3, vec.capacity);

	ASSERT_EQ(1, *(int *)cx_vec_get(&vec, 0));
	ASSERT_EQ(2, *(int *)cx_vec_get(&vec, 1));
	ASSERT_EQ(3, *(int *)cx_vec_get(&vec, 2));

	cx_vec_cleanup(&vec);
}

static void
peek_empty(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int));
	ASSERT_EQ(0, rv);

	ASSERT_NULL(cx_vec_peek(&vec));

	cx_vec_cleanup(&vec);
}

static void
peek_returns_last(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int));
	ASSERT_EQ(0, rv);

	int a = 11, b = 22, c = 33;
	void *p = cx_vec_push(&vec, &a);
	ASSERT_NOT_NULL(p);
	ASSERT_EQ(11, *(int *)cx_vec_peek(&vec));
	p = cx_vec_push(&vec, &b);
	ASSERT_NOT_NULL(p);
	ASSERT_EQ(22, *(int *)cx_vec_peek(&vec));
	p = cx_vec_push(&vec, &c);
	ASSERT_NOT_NULL(p);
	ASSERT_EQ(33, *(int *)cx_vec_peek(&vec));

	cx_vec_cleanup(&vec);
}

static void
pull_returns_last_and_shrinks(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int));
	ASSERT_EQ(0, rv);

	int a = 1, b = 2, c = 3;
	void *p = cx_vec_push(&vec, &a);
	ASSERT_NOT_NULL(p);
	p = cx_vec_push(&vec, &b);
	ASSERT_NOT_NULL(p);
	p = cx_vec_push(&vec, &c);
	ASSERT_NOT_NULL(p);
	ASSERT_EQ(3, vec.size);

	int out = 0;
	void *pulled = cx_vec_pull(&vec, &out);
	ASSERT_EQ(&out, pulled);
	ASSERT_EQ(3, out);
	ASSERT_EQ(2, vec.size);

	pulled = cx_vec_pull(&vec, &out);
	ASSERT_EQ(&out, pulled);
	ASSERT_EQ(2, out);
	ASSERT_EQ(1, vec.size);

	pulled = cx_vec_pull(&vec, &out);
	ASSERT_EQ(&out, pulled);
	ASSERT_EQ(1, out);
	ASSERT_EQ(0, vec.size);

	cx_vec_cleanup(&vec);
}

static void
pull_empty(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int));
	ASSERT_EQ(0, rv);

	int out = 99;
	void *pulled = cx_vec_pull(&vec, &out);
	ASSERT_NULL(pulled);
	ASSERT_EQ(99, out);

	cx_vec_cleanup(&vec);
}

DECLARE_TESTS
TEST(init_vec)
TEST(reserve_capacity)
TEST(push_and_get)
TEST(get_out_of_bounds)
TEST(push_grows_capacity)
TEST(peek_empty)
TEST(peek_returns_last)
TEST(pull_returns_last_and_shrinks)
TEST(pull_empty)
END_TESTS
