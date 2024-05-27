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
 * @file         rc_map.c
 */

#include "../../include/cextras/collection.h"

void
cx_rc_radix_tree_init(
		struct CxRcRadixTree *map, sqsh_rc_map_cleanup_t cleanup,
		size_t element_size) {
	map->cleanup = cleanup;
	cx_radix_tree_init(&map->inner, element_size + sizeof(struct CxRc));
}

const void *
cx_rc_radix_tree_put(
		struct CxRcRadixTree *map, uint64_t key, const void *value) {
	struct CxRc *rc = cx_radix_tree_put(&map->inner, key, value);
	if (!rc) {
		return NULL;
	}

	cx_rc_init(rc);

	return (void *)&rc[1];
}

int
cx_rc_radix_tree_release(struct CxRcRadixTree *map, uint64_t key) {
	int rv = 0;
	struct CxRc *rc = cx_radix_tree_get(&map->inner, key);
	if (!rc) {
		return -1;
	}

	if (cx_rc_release(rc)) {
		rv = cx_radix_tree_delete(&map->inner, key);
	}
	return rv;
}

const void *
cx_rc_radix_tree_retain(struct CxRcRadixTree *map, uint64_t key) {
	struct CxRc *rc = cx_radix_tree_get(&map->inner, key);
	if (!rc) {
		return NULL;
	}

	cx_rc_retain(rc);
	return (void *)&rc[1];
}

void
cx_rc_radix_tree_cleanup(struct CxRcRadixTree *map) {
	cx_radix_tree_cleanup(&map->inner);
}

static const void *
lru_rc_radix_tree_retain(void *backend, uint64_t index) {
	return cx_rc_radix_tree_retain(backend, index);
}

static int
lru_rc_radix_tree_release(void *backend, uint64_t index) {
	return cx_rc_radix_tree_release(backend, index);
}

const struct CxLruBackendImpl cx_lru_rc_radix_tree = {
		.retain = lru_rc_radix_tree_retain,
		.release = lru_rc_radix_tree_release,
};
