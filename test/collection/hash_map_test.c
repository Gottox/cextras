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

static void
init_hash_map(void) {
	int rv;
	struct CxHashMap map;

	rv = cx_hash_map_init(&map, 128, sizeof(uint8_t));
	ASSERT_EQ(0, rv);

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
set_and_get_element(void) {
	int rv;
	struct CxHashMap map;
	uint8_t data = 23;

	rv = cx_hash_map_init(&map, 128, sizeof(uint8_t));
	ASSERT_EQ(0, rv);

	uint64_t key = 4242424;
	const uint8_t *set_ptr = cx_hash_map_put(&map, key, &data);
	ASSERT_NOT_NULL(set_ptr);
	ASSERT_NE(set_ptr, &data);

	const uint8_t *get_ptr = cx_hash_map_get(&map, key);
	ASSERT_NOT_NULL(get_ptr);
	ASSERT_NE(get_ptr, &data);
	ASSERT_EQ(data, *get_ptr);

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
check_overflow(void) {
	int rv;
	struct CxHashMap map;
	uint8_t data = 1;
	const uint8_t *ptr;

	rv = cx_hash_map_init(&map, 4, sizeof(uint8_t));
	ASSERT_EQ(0, rv);

	ptr = cx_hash_map_put(&map, 1, &data);
	ASSERT_EQ(data, *ptr);
	ptr = cx_hash_map_put(&map, 2, &data);
	ASSERT_EQ(data, *ptr);
	ptr = cx_hash_map_put(&map, 3, &data);
	ASSERT_EQ(data, *ptr);
	ptr = cx_hash_map_put(&map, 4, &data);
	ASSERT_EQ(data, *ptr);
	data = 42;
	ptr = cx_hash_map_put(&map, 5, &data);
	ASSERT_EQ(data, *ptr);

	const uint8_t *get_ptr = cx_hash_map_get(&map, 5);
	ASSERT_NOT_NULL(get_ptr);
	ASSERT_EQ(42, *get_ptr);

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
stress_test_many_elements(void) {
	int rv;
	struct CxHashMap map;
	const int count = 10000;

	rv = cx_hash_map_init(&map, 16, sizeof(uint64_t));
	ASSERT_EQ(0, rv);

	for (int i = 0; i < count; i++) {
		uint64_t data = (uint64_t)i * 100;
		const uint64_t *ptr = cx_hash_map_put(&map, (uint64_t)i, &data);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ(data, *ptr);
	}

	for (int i = 0; i < count; i++) {
		uint64_t *ptr = cx_hash_map_get(&map, (uint64_t)i);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ((uint64_t)i * 100, *ptr);
		cx_hash_map_delete(&map, i);
	}

	for (int i = 0; i < count; i++) {
		cx_hash_map_delete(&map, (uint64_t)i);
	}

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_duplicate_key(void) {
	int rv;
	struct CxHashMap map;
	uint8_t data1 = 10;
	uint8_t data2 = 20;

	rv = cx_hash_map_init(&map, 16, sizeof(uint8_t));
	ASSERT_EQ(0, rv);

	const uint8_t *ptr1 = cx_hash_map_put(&map, 42, &data1);
	ASSERT_NOT_NULL(ptr1);
	ASSERT_EQ(10, *ptr1);

	const uint8_t *ptr2 = cx_hash_map_put(&map, 42, &data2);
	ASSERT_NOT_NULL(ptr2);
	ASSERT_EQ(ptr1, ptr2);
	ASSERT_EQ(10, *ptr2);

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_collision_handling(void) {
	int rv;
	struct CxHashMap map;
	const int count = 50;

	rv = cx_hash_map_init(&map, 8, sizeof(uint64_t));
	ASSERT_EQ(0, rv);

	for (int i = 0; i < count; i++) {
		uint64_t data = (uint64_t)i * 7;
		const uint64_t *ptr = cx_hash_map_put(&map, (uint64_t)i, &data);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ(data, *ptr);
	}

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_get_nonexistent_key(void) {
	int rv;
	struct CxHashMap map;

	rv = cx_hash_map_init(&map, 16, sizeof(uint64_t));
	ASSERT_EQ(0, rv);

	/* Get from empty map */
	void *ptr = cx_hash_map_get(&map, 12345);
	ASSERT_NULL(ptr);

	/* Add some elements */
	uint64_t data = 42;
	cx_hash_map_put(&map, 1, &data);
	cx_hash_map_put(&map, 2, &data);
	cx_hash_map_put(&map, 3, &data);

	/* Get non-existent key from non-empty map */
	ptr = cx_hash_map_get(&map, 999);
	ASSERT_NULL(ptr);

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_delete_nonexistent_key(void) {
	int rv;
	struct CxHashMap map;

	rv = cx_hash_map_init(&map, 16, sizeof(uint64_t));
	ASSERT_EQ(0, rv);

	/* Delete from empty map should succeed (no-op) */
	rv = cx_hash_map_delete(&map, 12345);
	ASSERT_EQ(0, rv);

	/* Add and delete an element */
	uint64_t data = 42;
	cx_hash_map_put(&map, 1, &data);
	rv = cx_hash_map_delete(&map, 1);
	ASSERT_EQ(0, rv);

	/* Deleting again should succeed (no-op) */
	rv = cx_hash_map_delete(&map, 1);
	ASSERT_EQ(0, rv);

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_hash_map_size(void) {
	int rv;
	struct CxHashMap map;

	rv = cx_hash_map_init(&map, 32, sizeof(uint64_t));
	ASSERT_EQ(0, rv);

	size_t size = cx_hash_map_size(&map);
	ASSERT_EQ((size_t)32, size);

	/* Add elements - capacity may grow */
	for (int i = 0; i < 50; i++) {
		uint64_t data = (uint64_t)i;
		cx_hash_map_put(&map, (uint64_t)i, &data);
	}

	/* Size should have grown */
	size = cx_hash_map_size(&map);
	ASSERT_TRUE(size > 32);

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_delete_and_reinsert(void) {
	int rv;
	struct CxHashMap map;

	rv = cx_hash_map_init(&map, 16, sizeof(uint64_t));
	ASSERT_EQ(0, rv);

	/* Insert elements */
	for (int i = 0; i < 10; i++) {
		uint64_t data = (uint64_t)i * 10;
		cx_hash_map_put(&map, (uint64_t)i, &data);
	}

	/* Delete some elements (creates tombstones) */
	cx_hash_map_delete(&map, 3);
	cx_hash_map_delete(&map, 5);
	cx_hash_map_delete(&map, 7);

	/* Verify deleted elements are gone */
	ASSERT_NULL(cx_hash_map_get(&map, 3));
	ASSERT_NULL(cx_hash_map_get(&map, 5));
	ASSERT_NULL(cx_hash_map_get(&map, 7));

	/* Verify remaining elements are still accessible */
	uint64_t *ptr = cx_hash_map_get(&map, 4);
	ASSERT_NOT_NULL(ptr);
	ASSERT_EQ((uint64_t)40, *ptr);

	/* Reinsert at deleted positions (should reuse tombstones) */
	uint64_t new_data = 333;
	ptr = cx_hash_map_put(&map, 3, &new_data);
	ASSERT_NOT_NULL(ptr);
	ASSERT_EQ((uint64_t)333, *ptr);

	new_data = 555;
	ptr = cx_hash_map_put(&map, 5, &new_data);
	ASSERT_NOT_NULL(ptr);
	ASSERT_EQ((uint64_t)555, *ptr);

	/* Verify reinserted elements */
	ptr = cx_hash_map_get(&map, 3);
	ASSERT_NOT_NULL(ptr);
	ASSERT_EQ((uint64_t)333, *ptr);

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_high_collision_robin_hood(void) {
	int rv;
	struct CxHashMap map;

	/* Very small initial capacity to force many collisions and robin hood swaps */
	rv = cx_hash_map_init(&map, 4, sizeof(uint64_t));
	ASSERT_EQ(0, rv);

	/* Insert many elements to trigger multiple resizes and robin hood swaps */
	for (int i = 0; i < 200; i++) {
		uint64_t data = (uint64_t)i * 3;
		const uint64_t *ptr = cx_hash_map_put(&map, (uint64_t)i, &data);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ(data, *ptr);
	}

	/* Verify all elements */
	for (int i = 0; i < 200; i++) {
		uint64_t *ptr = cx_hash_map_get(&map, (uint64_t)i);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ((uint64_t)i * 3, *ptr);
	}

	/* Delete every other element */
	for (int i = 0; i < 200; i += 2) {
		rv = cx_hash_map_delete(&map, (uint64_t)i);
		ASSERT_EQ(0, rv);
	}

	/* Verify remaining elements still accessible */
	for (int i = 1; i < 200; i += 2) {
		uint64_t *ptr = cx_hash_map_get(&map, (uint64_t)i);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ((uint64_t)i * 3, *ptr);
	}

	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_various_element_sizes(void) {
	int rv;

	/* Test with 1-byte elements */
	{
		struct CxHashMap map;
		rv = cx_hash_map_init(&map, 16, sizeof(uint8_t));
		ASSERT_EQ(0, rv);

		uint8_t data = 255;
		uint8_t *ptr = cx_hash_map_put(&map, 1, &data);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ((uint8_t)255, *ptr);

		cx_hash_map_cleanup(&map);
	}

	/* Test with larger struct elements */
	{
		struct CxHashMap map;
		struct LargeData {
			uint64_t a;
			uint64_t b;
			uint64_t c;
		};
		struct LargeData large_data = {.a = 111, .b = 222, .c = 333};

		rv = cx_hash_map_init(&map, 16, sizeof(large_data));
		ASSERT_EQ(0, rv);

		void *ptr = cx_hash_map_put(&map, 1, &large_data);
		ASSERT_NOT_NULL(ptr);

		struct LargeData *got = cx_hash_map_get(&map, 1);
		ASSERT_NOT_NULL(got);
		ASSERT_EQ(large_data.a, got->a);
		ASSERT_EQ(large_data.b, got->b);
		ASSERT_EQ(large_data.c, got->c);

		cx_hash_map_cleanup(&map);
	}
}

static void
test_operations_after_cleanup(void) {
	int rv;
	struct CxHashMap map;

	rv = cx_hash_map_init(&map, 16, sizeof(uint64_t));
	ASSERT_EQ(0, rv);

	/* Add an element */
	uint64_t data = 42;
	cx_hash_map_put(&map, 1, &data);

	/* Cleanup the map */
	rv = cx_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);

	/* After cleanup, capacity should be 0, so operations should handle this */
	void *ptr = cx_hash_map_get(&map, 1);
	ASSERT_NULL(ptr);

	rv = cx_hash_map_delete(&map, 1);
	ASSERT_EQ(0, rv);

	/* Size should be 0 */
	size_t size = cx_hash_map_size(&map);
	ASSERT_EQ((size_t)0, size);
}

DECLARE_TESTS
TEST(init_hash_map)
TEST(set_and_get_element)
TEST(check_overflow)
TEST(stress_test_many_elements)
TEST(test_duplicate_key)
TEST(test_collision_handling)
TEST(test_get_nonexistent_key)
TEST(test_delete_nonexistent_key)
TEST(test_hash_map_size)
TEST(test_delete_and_reinsert)
TEST(test_high_collision_robin_hood)
TEST(test_various_element_sizes)
TEST(test_operations_after_cleanup)
END_TESTS
