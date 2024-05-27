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

#include <assert.h>
#include <cextras/collection.h>
#include <stdint.h>
#include <testlib.h>
#include <time.h>

static void
init_radix_tree(void) {
	struct CxRadixTree map;

	cx_radix_tree_init(&map, sizeof(uintptr_t));

	cx_radix_tree_cleanup(&map);
}

static void
set_and_get_element(void) {
	int rv = 0;
	struct CxRadixTree map;

	cx_radix_tree_init(&map, sizeof(uintptr_t));

	uint64_t key = 4242424;
	uintptr_t value = 42;
	const uintptr_t *set_ptr = cx_radix_tree_put(&map, key, &value);
	assert(set_ptr != NULL);
	assert(*set_ptr == 42);

	const uintptr_t *get_ptr = cx_radix_tree_get(&map, key);
	assert(get_ptr == set_ptr);
	assert(*get_ptr == 42);

	rv = cx_radix_tree_delete(&map, key);
	assert(rv == 0);

	cx_radix_tree_cleanup(&map);
}

static void
set_size_max(void) {
	int rv = 0;
	struct CxRadixTree map;

	cx_radix_tree_init(&map, sizeof(uintptr_t));

	uint64_t key = SIZE_MAX;
	uintptr_t value = 42;
	const uintptr_t *set_ptr = cx_radix_tree_put(&map, key, &value);
	assert(set_ptr != NULL);
	assert(*set_ptr == 42);

	const uintptr_t *get_ptr = cx_radix_tree_get(&map, key);
	assert(get_ptr == set_ptr);
	assert(*get_ptr == 42);

	rv = cx_radix_tree_delete(&map, key);
	assert(rv == 0);

	cx_radix_tree_cleanup(&map);
}

DECLARE_TESTS
TEST(init_radix_tree)
TEST(set_and_get_element)
TEST(set_size_max)
END_TESTS
