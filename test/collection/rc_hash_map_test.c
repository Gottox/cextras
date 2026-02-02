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
 * @file         rc_map.c
 */

#include <cextras/collection.h>
#include <stdatomic.h>
#include <stdint.h>
#include <testlib.h>
#include <time.h>

static atomic_uint rc_hash_map_deinit_calls = 0;

static void
rc_hash_map_deinit(void *data) {
	uint8_t *data_ptr = data;
	*data_ptr = UINT8_MAX;
	rc_hash_map_deinit_calls++;
}

static void
init_rc_hash_map(void) {
	int rv;
	struct CxRcHashMap map;

	rv = cx_rc_hash_map_init(&map, 128, sizeof(uint8_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
set_and_get_element(void) {
	int rv;
	struct CxRcHashMap map;
	uint8_t data = 23;

	rv = cx_rc_hash_map_init(&map, 128, sizeof(uint8_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	uint64_t key = 4242424;
	const uint8_t *set_ptr = cx_rc_hash_map_put(&map, key, &data);
	ASSERT_NOT_NULL(set_ptr);
	ASSERT_NE(set_ptr, &data);

	const uint8_t *get_ptr = cx_rc_hash_map_retain(&map, key);
	ASSERT_NOT_NULL(get_ptr);
	ASSERT_NE(get_ptr, &data);
	ASSERT_EQ(data, *get_ptr);

	cx_rc_hash_map_release(&map, set_ptr);
	cx_rc_hash_map_release(&map, get_ptr);

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
check_overflow(void) {
	int rv;
	struct CxRcHashMap map;
	uint8_t data = 1;
	const uint8_t *ptr;

	rv = cx_rc_hash_map_init(&map, 4, sizeof(uint8_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	ptr = cx_rc_hash_map_put(&map, 1, &data);
	ASSERT_EQ(data, *ptr);
	ptr = cx_rc_hash_map_put(&map, 2, &data);
	ASSERT_EQ(data, *ptr);
	ptr = cx_rc_hash_map_put(&map, 3, &data);
	ASSERT_EQ(data, *ptr);
	ptr = cx_rc_hash_map_put(&map, 4, &data);
	ASSERT_EQ(data, *ptr);
	data = 42;
	ptr = cx_rc_hash_map_put(&map, 5, &data);
	ASSERT_EQ(data, *ptr);

	const uint8_t *get_ptr = cx_rc_hash_map_retain(&map, 5);
	ASSERT_NOT_NULL(get_ptr);
	ASSERT_EQ(42, *get_ptr);
	cx_rc_hash_map_release(&map, get_ptr);

	cx_rc_hash_map_release_key(&map, 1);
	cx_rc_hash_map_release_key(&map, 2);
	cx_rc_hash_map_release_key(&map, 3);
	cx_rc_hash_map_release_key(&map, 4);
	cx_rc_hash_map_release_key(&map, 5);

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
stress_test_many_elements(void) {
	int rv;
	struct CxRcHashMap map;
	const int count = 10000;

	rv = cx_rc_hash_map_init(&map, 16, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	for (int i = 0; i < count; i++) {
		uint64_t data = (uint64_t)i * 100;
		const uint64_t *ptr = cx_rc_hash_map_put(&map, (uint64_t)i, &data);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ(data, *ptr);
	}

	for (int i = 0; i < count; i++) {
		uint64_t *ptr = cx_rc_hash_map_retain(&map, (uint64_t)i);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ((uint64_t)i * 100, *ptr);
		cx_rc_hash_map_release(&map, ptr);
	}

	for (int i = 0; i < count; i++) {
		cx_rc_hash_map_release_key(&map, (uint64_t)i);
	}

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
stress_test_resize_with_held_refs(void) {
	int rv;
	struct CxRcHashMap map;
	const int count = 100;
	const uint64_t *held_refs[count];

	rv = cx_rc_hash_map_init(&map, 8, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	for (int i = 0; i < count; i++) {
		uint64_t data = (uint64_t)i;
		const uint64_t *ptr = cx_rc_hash_map_put(&map, (uint64_t)i, &data);
		ASSERT_NOT_NULL(ptr);
		held_refs[i] = ptr;
	}

	for (int i = 0; i < count; i++) {
		ASSERT_EQ((uint64_t)i, *held_refs[i]);
	}

	for (int i = 0; i < count; i++) {
		uint64_t *ptr = cx_rc_hash_map_retain(&map, (uint64_t)i);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ((uint64_t)i, *ptr);
		cx_rc_hash_map_release(&map, ptr);
	}

	for (int i = 0; i < count; i++) {
		cx_rc_hash_map_release(&map, held_refs[i]);
	}

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_duplicate_key(void) {
	int rv;
	struct CxRcHashMap map;
	uint8_t data1 = 10;
	uint8_t data2 = 20;

	rv = cx_rc_hash_map_init(&map, 16, sizeof(uint8_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	const uint8_t *ptr1 = cx_rc_hash_map_put(&map, 42, &data1);
	ASSERT_NOT_NULL(ptr1);
	ASSERT_EQ(10, *ptr1);

	const uint8_t *ptr2 = cx_rc_hash_map_put(&map, 42, &data2);
	ASSERT_NOT_NULL(ptr2);
	ASSERT_EQ(ptr1, ptr2);
	ASSERT_EQ(10, *ptr2);

	cx_rc_hash_map_release(&map, ptr1);
	cx_rc_hash_map_release(&map, ptr2);

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_collision_handling(void) {
	int rv;
	struct CxRcHashMap map;
	const int count = 50;

	rv = cx_rc_hash_map_init(&map, 8, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	for (int i = 0; i < count; i++) {
		uint64_t data = (uint64_t)i * 7;
		const uint64_t *ptr = cx_rc_hash_map_put(&map, (uint64_t)i, &data);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ(data, *ptr);
	}

	for (int i = count - 1; i >= 0; i--) {
		uint64_t *ptr = cx_rc_hash_map_retain(&map, (uint64_t)i);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ((uint64_t)i * 7, *ptr);
		cx_rc_hash_map_release(&map, ptr);
	}

	for (int i = 0; i < count; i++) {
		cx_rc_hash_map_release_key(&map, (uint64_t)i);
	}

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

DECLARE_TESTS
TEST(init_rc_hash_map)
TEST(set_and_get_element)
TEST(check_overflow)
TEST(stress_test_many_elements)
TEST(stress_test_resize_with_held_refs)
TEST(test_duplicate_key)
TEST(test_collision_handling)
END_TESTS
