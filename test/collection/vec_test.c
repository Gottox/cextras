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

#include <assert.h>
#include <cextras/collection.h>
#include <stdint.h>
#include <string.h>
#include <testlib.h>

static void
init_vec(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int), 0);
	assert(rv == 0);
	assert(vec.size == 0);
	assert(vec.capacity == 16);
	assert(vec.element_size == sizeof(int));
	assert(vec.data != NULL);

	cx_vec_cleanup(&vec);
	assert(vec.data == NULL);
	assert(vec.size == 0);
	assert(vec.capacity == 0);
	assert(vec.element_size == 0);
}

static void
init_vec_with_capacity(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(uint64_t), 4);
	assert(rv == 0);
	assert(vec.capacity == 4);
	assert(vec.element_size == sizeof(uint64_t));

	cx_vec_cleanup(&vec);
}

static void
push_and_get(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int), 4);
	assert(rv == 0);

	for (int i = 0; i < 4; i++) {
		int v = i * 10;
		void *p = cx_vec_push(&vec, &v);
		assert(p != NULL);
		assert(*(int *)p == v);
	}
	assert(vec.size == 4);
	assert(vec.capacity == 4);

	for (int i = 0; i < 4; i++) {
		int *p = cx_vec_get(&vec, (size_t)i);
		assert(p != NULL);
		assert(*p == i * 10);
	}

	cx_vec_cleanup(&vec);
}

static void
get_out_of_bounds(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int), 4);
	assert(rv == 0);

	assert(cx_vec_get(&vec, 0) == NULL);

	int v = 42;
	void *p = cx_vec_push(&vec, &v);
	assert(p != NULL);

	assert(cx_vec_get(&vec, 0) != NULL);
	assert(cx_vec_get(&vec, 1) == NULL);
	assert(cx_vec_get(&vec, 100) == NULL);

	cx_vec_cleanup(&vec);
}

static void
push_grows_capacity(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int), 2);
	assert(rv == 0);
	assert(vec.capacity == 2);

	int a = 1, b = 2, c = 3;
	assert(cx_vec_push(&vec, &a) != NULL);
	assert(cx_vec_push(&vec, &b) != NULL);
	assert(vec.capacity == 2);

	assert(cx_vec_push(&vec, &c) != NULL);
	assert(vec.size == 3);
	assert(vec.capacity >= 3);

	assert(*(int *)cx_vec_get(&vec, 0) == 1);
	assert(*(int *)cx_vec_get(&vec, 1) == 2);
	assert(*(int *)cx_vec_get(&vec, 2) == 3);

	cx_vec_cleanup(&vec);
}

static void
peek_empty(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int), 4);
	assert(rv == 0);

	assert(cx_vec_peek(&vec) == NULL);

	cx_vec_cleanup(&vec);
}

static void
peek_returns_last(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int), 4);
	assert(rv == 0);

	int a = 11, b = 22, c = 33;
	assert(cx_vec_push(&vec, &a) != NULL);
	assert(*(int *)cx_vec_peek(&vec) == 11);
	assert(cx_vec_push(&vec, &b) != NULL);
	assert(*(int *)cx_vec_peek(&vec) == 22);
	assert(cx_vec_push(&vec, &c) != NULL);
	assert(*(int *)cx_vec_peek(&vec) == 33);

	cx_vec_cleanup(&vec);
}

static void
pull_returns_last_and_shrinks(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int), 4);
	assert(rv == 0);

	int a = 1, b = 2, c = 3;
	assert(cx_vec_push(&vec, &a) != NULL);
	assert(cx_vec_push(&vec, &b) != NULL);
	assert(cx_vec_push(&vec, &c) != NULL);
	assert(vec.size == 3);

	int out = 0;
	assert(cx_vec_pull(&vec, &out) == &out);
	assert(out == 3);
	assert(vec.size == 2);

	assert(cx_vec_pull(&vec, &out) == &out);
	assert(out == 2);
	assert(vec.size == 1);

	assert(cx_vec_pull(&vec, &out) == &out);
	assert(out == 1);
	assert(vec.size == 0);

	cx_vec_cleanup(&vec);
}

static void
pull_empty(void) {
	struct CxVec vec = {0};

	int rv = cx_vec_init(&vec, sizeof(int), 4);
	assert(rv == 0);

	int out = 99;
	assert(cx_vec_pull(&vec, &out) == NULL);
	assert(out == 99);

	cx_vec_cleanup(&vec);
}

DECLARE_TESTS
TEST(init_vec)
TEST(init_vec_with_capacity)
TEST(push_and_get)
TEST(get_out_of_bounds)
TEST(push_grows_capacity)
TEST(peek_empty)
TEST(peek_returns_last)
TEST(pull_returns_last_and_shrinks)
TEST(pull_empty)
END_TESTS
