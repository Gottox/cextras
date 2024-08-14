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

#define EMPTY_MARKER SIZE_MAX

#if 0
#	include <stdio.h>

static void
debug_print(const struct CxLru *lru, const char msg, size_t ring_index) {
	size_t backend_index = lru->items[ring_index];

	fprintf(stderr, "%clru %lu: ", msg, ring_index);
	size_t sum = 0;
	for (size_t i = 0; i < lru->size; i++) {
		size_t cur_index = lru->items[i];
		if (cur_index == backend_index) {
			sum++;
		}
	}
	fprintf(stderr, "idx: %lu refs: %lu\n", backend_index, sum);
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

	lru->items = calloc(size, sizeof(size_t));
	if (lru->items == NULL) {
		return -CX_ERR_ALLOC;
	}
	for (size_t i = 0; i < lru->size; i++) {
		lru->items[i] = EMPTY_MARKER;
	}
	lru->ring_index = 0;

	return 0;
}

int
cx_lru_touch(struct CxLru *lru, uint64_t index) {
	if (lru->size == 0) {
		return 0;
	}

	size_t ring_index = lru->ring_index;
	size_t size = lru->size;
	void *backend = lru->backend;
	const struct CxLruBackendImpl *impl = lru->impl;
	size_t last_index = lru->items[ring_index];

	ring_index = (ring_index + 1) % size;

	size_t old_index = lru->items[ring_index];

	if (old_index == index || last_index == index) {
		return 0;
	}

	debug_print(lru, '-', ring_index);
	if (old_index != EMPTY_MARKER) {
		impl->release(backend, old_index);
	}

	lru->items[ring_index] = index;
	debug_print(lru, '+', ring_index);
	impl->retain(backend, index);

	lru->ring_index = ring_index;

	return 0;
}

int
cx_lru_cleanup(struct CxLru *lru) {
	const struct CxLruBackendImpl *impl = lru->impl;

	for (size_t i = 0; i < lru->size; i++) {
		size_t index = lru->items[i];
		if (index != EMPTY_MARKER) {
			impl->release(lru->backend, index);
		}
	}
	free(lru->items);
	lru->size = 0;
	lru->items = NULL;
	return 0;
}
