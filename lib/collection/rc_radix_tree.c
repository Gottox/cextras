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
#include <assert.h>

#define RC_MAP_SIZE 256

static uint32_t *
get_rc(struct CxRcRadixTree *tree, uint64_t key) {
	uint64_t inner_key = key & 0xff;
	uint64_t outer_key = key >> 8;

	uint32_t *rc_map = cx_radix_tree_get(&tree->rc, outer_key);
	if (rc_map == NULL) {
		rc_map = cx_radix_tree_new_leaf(&tree->rc, outer_key);
	}
	assert(rc_map != NULL);
	return &rc_map[inner_key];
}

int
cx_rc_radix_tree_init(
		struct CxRcRadixTree *tree, size_t element_size,
		sqsh_rc_map_cleanup_t cleanup) {
	tree->cleanup = cleanup;
	tree->element_size = element_size;
	cx_radix_tree_init(&tree->values, element_size);
	cx_radix_tree_init(&tree->rc, sizeof(uint32_t[RC_MAP_SIZE]));
	return 0;
}

const void *
cx_rc_radix_tree_put(struct CxRcRadixTree *tree, uint64_t key, void *value) {
	uint32_t *rc = get_rc(tree, key);
	if (*rc != 0) {
		(*rc)++;
		tree->cleanup(value);
		return cx_radix_tree_get(&tree->values, key);
	} else {
		*rc = 1;
		return cx_radix_tree_put(&tree->values, key, value);
	}
}

int
cx_rc_radix_tree_release(struct CxRcRadixTree *tree, uint64_t key) {
	int rv = 0;
	uint32_t *rc = get_rc(tree, key);
	if (*rc == 0) {
		return -1;
	} else if (*rc == 1) {
		tree->cleanup(cx_radix_tree_get(&tree->values, key));
		rv = cx_radix_tree_delete(&tree->values, key);
	}

	(*rc)--;
	return rv;
}

const void *
cx_rc_radix_tree_retain(struct CxRcRadixTree *tree, uint64_t key) {
	uint32_t *rc = get_rc(tree, key);

	if (*rc == 0) {
		return NULL;
	}

	(*rc)++;
	return cx_radix_tree_get(&tree->values, key);
}

int
cx_rc_radix_tree_cleanup(struct CxRcRadixTree *tree) {
	cx_radix_tree_cleanup(&tree->values);
	cx_radix_tree_cleanup(&tree->rc);
	return 0;
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
