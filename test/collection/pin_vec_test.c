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
 * @file         pin_vec_test.c
 */

#include <cextras/collection.h>
#include <stdint.h>
#include <string.h>
#include <testlib.h>

struct Payload {
	uint64_t marker;
	uint64_t index;
};

static void
init_basic(void) {
	struct CxPinVec vec = {0};

	int rv = cx_pin_vec_init(&vec, sizeof(struct Payload), 4);
	ASSERT_EQ(0, rv);
	ASSERT_EQ((size_t)0, cx_pin_vec_size(&vec));
	ASSERT_NULL(cx_pin_vec_peek(&vec));

	cx_pin_vec_cleanup(&vec);
}

static void
elements_stay_pinned_across_vec_reallocation(void) {
	struct CxPinVec vec = {0};

	int rv = cx_pin_vec_init(&vec, sizeof(struct Payload), 2);
	ASSERT_EQ(0, rv);
	ASSERT_EQ((size_t)2, vec.vec.capacity);

	enum { N = 64 };
	struct Payload *saved[N];

	for (size_t i = 0; i < N; i++) {
		struct Payload in = {
				.marker = 0xDEADBEEF00000000ULL | i,
				.index = i,
		};
		struct Payload *p = cx_pin_vec_push(&vec, &in);
		ASSERT_NOT_NULL(p);
		ASSERT_EQ(in.marker, p->marker);
		ASSERT_EQ(in.index, p->index);
		saved[i] = p;
	}

	ASSERT_EQ((size_t)N, cx_pin_vec_size(&vec));
	ASSERT_GT(vec.vec.capacity, (size_t)2);

	for (size_t i = 0; i < N; i++) {
		struct Payload *p = cx_pin_vec_get(&vec, i);
		ASSERT_EQ(saved[i], p);
		ASSERT_EQ(0xDEADBEEF00000000ULL | i, p->marker);
		ASSERT_EQ(i, p->index);
	}

	cx_pin_vec_cleanup(&vec);
}

static void
mutation_via_saved_pointer_survives_reallocation(void) {
	struct CxPinVec vec = {0};

	int rv = cx_pin_vec_init(&vec, sizeof(struct Payload), 1);
	ASSERT_EQ(0, rv);

	struct Payload first_in = {.marker = 0x1111, .index = 0};
	struct Payload *first = cx_pin_vec_push(&vec, &first_in);
	ASSERT_NOT_NULL(first);
	ASSERT_EQ((uint64_t)0x1111, first->marker);

	for (size_t i = 1; i < 50; i++) {
		struct Payload in = {.marker = i, .index = i};
		ASSERT_NOT_NULL(cx_pin_vec_push(&vec, &in));
	}

	first->marker = 0x2222;
	first->index = 999;

	struct Payload *again = cx_pin_vec_get(&vec, 0);
	ASSERT_EQ(first, again);
	ASSERT_EQ((uint64_t)0x2222, again->marker);
	ASSERT_EQ((uint64_t)999, again->index);

	cx_pin_vec_cleanup(&vec);
}

static void
all_saved_pointers_remain_distinct_and_valid(void) {
	struct CxPinVec vec = {0};

	int rv = cx_pin_vec_init(&vec, sizeof(struct Payload), 2);
	ASSERT_EQ(0, rv);

	enum { N = 100 };
	struct Payload *saved[N];

	for (size_t i = 0; i < N; i++) {
		struct Payload in = {.marker = 0xA000 + i, .index = i};
		saved[i] = cx_pin_vec_push(&vec, &in);
		ASSERT_NOT_NULL(saved[i]);
	}

	for (size_t i = 0; i < N; i++) {
		for (size_t j = i + 1; j < N; j++) {
			ASSERT_NE(saved[i], saved[j]);
		}
	}

	for (size_t i = 0; i < N; i++) {
		ASSERT_EQ((uint64_t)(0xA000 + i), saved[i]->marker);
		ASSERT_EQ((uint64_t)i, saved[i]->index);
	}

	cx_pin_vec_cleanup(&vec);
}

static void
peek_returns_last_after_realloc(void) {
	struct CxPinVec vec = {0};

	int rv = cx_pin_vec_init(&vec, sizeof(struct Payload), 2);
	ASSERT_EQ(0, rv);

	for (size_t i = 0; i < 32; i++) {
		struct Payload in = {.marker = i + 1, .index = i};
		struct Payload *p = cx_pin_vec_push(&vec, &in);
		ASSERT_NOT_NULL(p);

		struct Payload *peek = cx_pin_vec_peek(&vec);
		ASSERT_EQ(p, peek);
		ASSERT_EQ((uint64_t)(i + 1), peek->marker);
	}

	cx_pin_vec_cleanup(&vec);
}

static void
pull_decrements_and_addresses_remain_valid(void) {
	struct CxPinVec vec = {0};

	int rv = cx_pin_vec_init(&vec, sizeof(struct Payload), 2);
	ASSERT_EQ(0, rv);

	enum { N = 20 };
	struct Payload *saved[N];

	for (size_t i = 0; i < N; i++) {
		struct Payload in = {.marker = 100 + i, .index = i};
		saved[i] = cx_pin_vec_push(&vec, &in);
		ASSERT_NOT_NULL(saved[i]);
	}
	ASSERT_EQ((size_t)N, cx_pin_vec_size(&vec));

	for (size_t i = 0; i < 5; i++) {
		ASSERT_EQ(0, cx_pin_vec_pull(&vec));
	}
	ASSERT_EQ((size_t)(N - 5), cx_pin_vec_size(&vec));

	for (size_t i = 0; i < N - 5; i++) {
		struct Payload *p = cx_pin_vec_get(&vec, i);
		ASSERT_EQ(saved[i], p);
		ASSERT_EQ((uint64_t)(100 + i), p->marker);
	}

	cx_pin_vec_cleanup(&vec);
}

static void
pull_empty_returns_error(void) {
	struct CxPinVec vec = {0};

	int rv = cx_pin_vec_init(&vec, sizeof(struct Payload), 4);
	ASSERT_EQ(0, rv);

	ASSERT_EQ(-1, cx_pin_vec_pull(&vec));

	cx_pin_vec_cleanup(&vec);
}

static void
get_out_of_bounds_returns_null(void) {
	struct CxPinVec vec = {0};

	int rv = cx_pin_vec_init(&vec, sizeof(struct Payload), 4);
	ASSERT_EQ(0, rv);

	ASSERT_NULL(cx_pin_vec_get(&vec, 0));
	ASSERT_NULL(cx_pin_vec_get(&vec, 100));

	struct Payload in = {.marker = 1, .index = 0};
	ASSERT_NOT_NULL(cx_pin_vec_push(&vec, &in));
	ASSERT_NOT_NULL(cx_pin_vec_get(&vec, 0));
	ASSERT_NULL(cx_pin_vec_get(&vec, 1));

	cx_pin_vec_cleanup(&vec);
}

DECLARE_TESTS
TEST(init_basic)
TEST(elements_stay_pinned_across_vec_reallocation)
TEST(mutation_via_saved_pointer_survives_reallocation)
TEST(all_saved_pointers_remain_distinct_and_valid)
TEST(peek_returns_last_after_realloc)
TEST(pull_decrements_and_addresses_remain_valid)
TEST(pull_empty_returns_error)
TEST(get_out_of_bounds_returns_null)
END_TESTS
