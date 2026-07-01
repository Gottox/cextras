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

static atomic_uint rc_radix_tree_deinit_calls = 0;

static void
rc_radix_tree_deinit(void *data) {
	uint64_t *data_ptr = data;
	*data_ptr = UINT8_MAX;
	rc_radix_tree_deinit_calls++;
}

static void
init_rc_radix_tree(void) {
	int rv;
	struct CxRcRadixTree tree;

	rv = cx_rc_radix_tree_init(&tree, sizeof(uint64_t), rc_radix_tree_deinit);
	ASSERT_EQ(0, rv);

	rv = cx_rc_radix_tree_cleanup(&tree);
	ASSERT_EQ(0, rv);
}

static void
set_and_get_element(void) {
	int rv;
	struct CxRcRadixTree tree;
	uint64_t data = 23;

	rv = cx_rc_radix_tree_init(&tree, sizeof(uint64_t), rc_radix_tree_deinit);
	ASSERT_EQ(0, rv);

	uint64_t key = 0;
	const uint64_t *set_ptr = cx_rc_radix_tree_put(&tree, key, &data);
	ASSERT_EQ(0, rv);
	ASSERT_NE(&data, set_ptr);

	const uint64_t *get_ptr = cx_rc_radix_tree_retain(&tree, key);
	ASSERT_EQ(0, rv);
	ASSERT_NE(&data, get_ptr);
	ASSERT_EQ(data, *get_ptr);

	cx_rc_radix_tree_release(&tree, key);
	cx_rc_radix_tree_release(&tree, key);

	rv = cx_rc_radix_tree_cleanup(&tree);
	ASSERT_EQ(0, rv);
}

static void
test_cleanup(void) {
	int rv;
	struct CxRcRadixTree tree;
	uint64_t data = 23;

	rv = cx_rc_radix_tree_init(&tree, sizeof(uint64_t), rc_radix_tree_deinit);
	ASSERT_EQ(0, rv);

	uint64_t key = 6762;
	const uint64_t *set_ptr = cx_rc_radix_tree_put(&tree, key, &data);
	ASSERT_EQ(0, rv);
	ASSERT_NE(&data, set_ptr);

	key = 12715;
	const uint64_t *set_ptr2 = cx_rc_radix_tree_put(&tree, key, &data);
	ASSERT_EQ(0, rv);
	ASSERT_NE(&data, set_ptr2);

	cx_rc_radix_tree_release(&tree, 6762);
	cx_rc_radix_tree_release(&tree, 12715);

	rv = cx_rc_radix_tree_cleanup(&tree);
	ASSERT_EQ(0, rv);
}

DECLARE_TESTS
TEST(init_rc_radix_tree)
TEST(set_and_get_element)
TEST(test_cleanup)
END_TESTS
