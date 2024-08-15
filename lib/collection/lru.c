/******************************************************************************
 *                                                                            *
 * Copyright (c) 2023, Enno Boland <g@s01.de>                                 *
 *                                                                            *
 * Redistribution and use in source and binary forms, with or without         *
 * modification, are permitted provided that the following conditions are     *
 * met:                                                                       *
 *                                                                            *
 * * Redistributions of source code must retain the above copyright notice,   *
 *   this list of conditions and the following disclaimer.                    *
 * * Redistributions in binary form must reproduce the above copyright        *
 *   notice, this list of conditions and the following disclaimer in the      *
 *   documentation and/or other materials provided with the distribution.     *
 *                                                                            *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS    *
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,  *
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR     *
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR          *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,      *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,        *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR         *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF     *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING       *
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               *
 *                                                                            *
 ******************************************************************************/

/**
 * @author       Enno Boland (mail@eboland.de)
 * @file         lru.c
 */

#include "../../include/cextras/collection.h"
#include "../../include/cextras/error.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define EMPTY_MARKER UINT64_MAX

#if 0
#	include <stdio.h>

static void
debug_print(const struct CxLru *lru, const char msg, size_t ring_index) {
	uint64_t backend_key = lru->items[ring_index];

	fprintf(stderr, "%clru %lu: ", msg, ring_index);
	size_t sum = 0;
	for (size_t i = 0; i < lru->size; i++) {
		uint64_t cur_key = lru->items[i];
		if (cur_key == backend_key) {
			sum++;
		}
	}
	fprintf(stderr, "idx: %lu refs: %lu\n", backend_key, sum);
	fflush(stderr);
}
#else
#	define debug_print(...)
#endif

int
cx_lru_init(
		struct CxLru *lru, size_t size, const struct CxLruBackendImpl *impl,
		void *backend) {
	memset(lru, 0, sizeof(*lru));
	lru->impl = impl;
	lru->backend = backend;
	lru->size = size;
	if (size == 0) {
		return 0;
	}

	lru->items = calloc(size, sizeof(uint64_t));
	if (lru->items == NULL) {
		return -CX_ERR_ALLOC;
	}
	lru->ring_index = 0;
	memset(lru->items, 0xff, size * sizeof(uint64_t));

	return 0;
}

static bool
try_touch(struct CxLru *lru, uint64_t key) {
	if (lru->size == 0) {
		return false;
	}

	size_t ring_index = lru->ring_index;
	size_t size = lru->size;
	void *backend = lru->backend;
	const struct CxLruBackendImpl *impl = lru->impl;
	uint64_t last_key = lru->items[ring_index];

	ring_index = (ring_index + 1) % size;

	uint64_t old_key = lru->items[ring_index];

	if (old_key == key || last_key == key) {
		return false;
	}

	debug_print(lru, '-', ring_index);
	if (old_key != EMPTY_MARKER) {
		impl->release(backend, old_key);
	}

	lru->items[ring_index] = key;
	lru->ring_index = ring_index;

	return true;
}

int
cx_lru_touch(struct CxLru *lru, uint64_t key) {
	if (try_touch(lru, key)) {
		lru->impl->retain(lru->backend, key);
	}
	return 0;
}

int
cx_lru_touch_value(struct CxLru *lru, uint64_t key, void *value) {
	if (try_touch(lru, key)) {
		lru->impl->retain_value(lru->backend, value);
	}
	return 0;
}

int
cx_lru_cleanup(struct CxLru *lru) {
	const struct CxLruBackendImpl *impl = lru->impl;

	for (size_t i = 0; i < lru->size; i++) {
		uint64_t key = lru->items[i];
		if (key != EMPTY_MARKER) {
			impl->release(lru->backend, key);
		}
	}
	free(lru->items);
	lru->size = 0;
	lru->items = NULL;
	return 0;
}
