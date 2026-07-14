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
stress_test_resize_with_held_refs(void) {
	int rv;
	struct CxRcHashMap map;
	const size_t count = 100;
	const uint64_t *held_refs[count];

	rv = cx_rc_hash_map_init(&map, 8, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	for (size_t i = 0; i < count; i++) {
		uint64_t data = (uint64_t)i;
		const uint64_t *ptr = cx_rc_hash_map_put(&map, (uint64_t)i, &data);
		ASSERT_NOT_NULL(ptr);
		held_refs[i] = ptr;
	}

	for (size_t i = 0; i < count; i++) {
		ASSERT_EQ((uint64_t)i, *held_refs[i]);
	}

	for (size_t i = 0; i < count; i++) {
		uint64_t *ptr = cx_rc_hash_map_retain(&map, (uint64_t)i);
		ASSERT_NOT_NULL(ptr);
		ASSERT_EQ((uint64_t)i, *ptr);
		cx_rc_hash_map_release(&map, ptr);
	}

	for (size_t i = 0; i < count; i++) {
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
test_rc_hash_map_size(void) {
	int rv;
	struct CxRcHashMap map;

	rv = cx_rc_hash_map_init(&map, 32, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	size_t size = cx_rc_hash_map_size(&map);
	ASSERT_EQ((size_t)32, size);

	/* Add elements - capacity may grow */
	for (size_t i = 0; i < 50; i++) {
		uint64_t data = (uint64_t)i;
		cx_rc_hash_map_put(&map, (uint64_t)i, &data);
	}

	/* Size should have grown */
	size = cx_rc_hash_map_size(&map);
	ASSERT_TRUE(size > 32);

	/* Clean up - release all */
	for (size_t i = 0; i < 50; i++) {
		cx_rc_hash_map_release_key(&map, (uint64_t)i);
	}

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_retain_nonexistent_key(void) {
	int rv;
	struct CxRcHashMap map;

	rv = cx_rc_hash_map_init(&map, 16, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	/* Retain from empty map returns NULL */
	void *ptr = cx_rc_hash_map_retain(&map, 12345);
	ASSERT_NULL(ptr);

	/* Add some elements */
	uint64_t data = 42;
	cx_rc_hash_map_put(&map, 1, &data);
	cx_rc_hash_map_put(&map, 2, &data);

	/* Retain non-existent key from non-empty map */
	ptr = cx_rc_hash_map_retain(&map, 999);
	ASSERT_NULL(ptr);

	/* Clean up */
	cx_rc_hash_map_release_key(&map, 1);
	cx_rc_hash_map_release_key(&map, 2);

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_release_key_nonexistent(void) {
	int rv;
	struct CxRcHashMap map;

	rv = cx_rc_hash_map_init(&map, 16, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	/* Release non-existent key should be safe (no-op) */
	rv = cx_rc_hash_map_release_key(&map, 12345);
	ASSERT_EQ(0, rv);

	/* Add and release an element */
	uint64_t data = 42;
	cx_rc_hash_map_put(&map, 1, &data);
	rv = cx_rc_hash_map_release_key(&map, 1);
	ASSERT_EQ(0, rv);

	/* Releasing again should be safe */
	rv = cx_rc_hash_map_release_key(&map, 1);
	ASSERT_EQ(0, rv);

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_multiple_retains_releases(void) {
	int rv;
	struct CxRcHashMap map;
	unsigned int initial_deinit_calls = rc_hash_map_deinit_calls;

	rv = cx_rc_hash_map_init(&map, 16, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	/* Put creates entry with ref_count = 0 */
	uint64_t data = 100;
	uint64_t *ptr = cx_rc_hash_map_put(&map, 1, &data);
	ASSERT_NOT_NULL(ptr);

	/* Retain increments ref_count to 1 */
	uint64_t *ptr1 = cx_rc_hash_map_retain(&map, 1);
	ASSERT_NOT_NULL(ptr1);
	ASSERT_EQ(ptr, ptr1);

	/* Retain again increments ref_count to 2 */
	uint64_t *ptr2 = cx_rc_hash_map_retain(&map, 1);
	ASSERT_NOT_NULL(ptr2);
	ASSERT_EQ(ptr, ptr2);

	/* Release decrements ref_count to 1 - element still alive */
	cx_rc_hash_map_release(&map, ptr1);
	ASSERT_EQ(initial_deinit_calls, rc_hash_map_deinit_calls);

	/* Element should still be accessible */
	uint64_t *ptr3 = cx_rc_hash_map_retain(&map, 1);
	ASSERT_NOT_NULL(ptr3);

	/* Release all remaining references */
	cx_rc_hash_map_release(&map, ptr2);
	cx_rc_hash_map_release(&map, ptr3);

	/* Final release (ref_count was 0 from put) triggers cleanup */
	cx_rc_hash_map_release_key(&map, 1);
	ASSERT_EQ(initial_deinit_calls + 1, rc_hash_map_deinit_calls);

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_lru_backend_interface(void) {
	int rv;
	struct CxRcHashMap map;
	unsigned int initial_deinit_calls = rc_hash_map_deinit_calls;

	rv = cx_rc_hash_map_init(&map, 16, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	/* Put some elements */
	uint64_t data1 = 111;
	uint64_t data2 = 222;
	cx_rc_hash_map_put(&map, 1, &data1);
	cx_rc_hash_map_put(&map, 2, &data2);

	/* Use LRU backend interface to retain */
	uint64_t *ptr = cx_lru_rc_hash_map.retain(&map, 1);
	ASSERT_NOT_NULL(ptr);
	ASSERT_EQ((uint64_t)111, *ptr);

	/* Use LRU backend interface to release */
	rv = cx_lru_rc_hash_map.release(&map, 1);
	ASSERT_EQ(0, rv);

	/* Release via LRU interface again (should cleanup since ref_count was 0) */
	rv = cx_lru_rc_hash_map.release(&map, 1);
	ASSERT_EQ(0, rv);
	ASSERT_EQ(initial_deinit_calls + 1, rc_hash_map_deinit_calls);

	/* Cleanup remaining elements */
	cx_rc_hash_map_release_key(&map, 2);

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

static void
test_put_duplicate_increments_refcount(void) {
	int rv;
	struct CxRcHashMap map;
	unsigned int initial_deinit_calls = rc_hash_map_deinit_calls;

	rv = cx_rc_hash_map_init(&map, 16, sizeof(uint64_t), rc_hash_map_deinit);
	ASSERT_EQ(0, rv);

	/* First put */
	uint64_t data1 = 100;
	uint64_t *ptr1 = cx_rc_hash_map_put(&map, 1, &data1);
	ASSERT_NOT_NULL(ptr1);
	ASSERT_EQ((uint64_t)100, *ptr1);

	/* Duplicate put - cleanup is called on new data, existing entry's refcount
	 * incremented */
	uint64_t data2 = 200;
	uint64_t *ptr2 = cx_rc_hash_map_put(&map, 1, &data2);
	ASSERT_NOT_NULL(ptr2);
	ASSERT_EQ(ptr1, ptr2);
	ASSERT_EQ((uint64_t)100, *ptr2); /* Original value preserved */
	/* Cleanup was called on data2 */
	ASSERT_EQ(initial_deinit_calls + 1, rc_hash_map_deinit_calls);

	/* Need two releases now (one from put's ref_count increment) */
	cx_rc_hash_map_release(&map, ptr1);
	/* Element still alive (ref_count was incremented by second put) */
	uint64_t *ptr3 = cx_rc_hash_map_retain(&map, 1);
	ASSERT_NOT_NULL(ptr3);

	cx_rc_hash_map_release(&map, ptr3);
	cx_rc_hash_map_release_key(&map, 1);

	rv = cx_rc_hash_map_cleanup(&map);
	ASSERT_EQ(0, rv);
}

DECLARE_TESTS
TEST(init_rc_hash_map)
TEST(set_and_get_element)
TEST(stress_test_resize_with_held_refs)
TEST(test_duplicate_key)
TEST(test_rc_hash_map_size)
TEST(test_retain_nonexistent_key)
TEST(test_release_key_nonexistent)
TEST(test_multiple_retains_releases)
TEST(test_lru_backend_interface)
TEST(test_put_duplicate_increments_refcount)
END_TESTS
