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
#include <string.h>

typedef size_t rc_t;
#define RC_SIZE sizeof(rc_t)
#define OBJ_RC(obj) (obj[0])
#define OBJ_DATA(obj) ((void *)(&obj[1]))

int
cx_rc_radix_tree_init(
		struct CxRcRadixTree *tree, size_t element_size,
		sqsh_rc_map_cleanup_t cleanup) {
	tree->cleanup = cleanup;
	tree->element_size = element_size;
	cx_radix_tree_init(&tree->values, RC_SIZE + element_size);
	return 0;
}

rc_t *
radix_tree_retain(struct CxRcRadixTree *tree, uint64_t key) {
	rc_t *obj = cx_radix_tree_get(&tree->values, key);

	if (obj == NULL) {
		return NULL;
	}

	assert(OBJ_RC(obj) > 0);

	OBJ_RC(obj)++;

	return obj;
}

void *
cx_rc_radix_tree_put(struct CxRcRadixTree *tree, uint64_t key, void *value) {
	rc_t *obj = radix_tree_retain(tree, key);
	if (obj == NULL) {
		obj = cx_radix_tree_new_leaf(&tree->values, key);
		assert(obj != NULL);
		memcpy(OBJ_DATA(obj), value, tree->element_size);
		OBJ_RC(obj) = 1;
	} else {
		tree->cleanup(value);
	}
	return OBJ_DATA(obj);
}

int
cx_rc_radix_tree_release(struct CxRcRadixTree *tree, uint64_t key) {
	int rv = 0;
	rc_t *obj = cx_radix_tree_get(&tree->values, key);
	if (obj == NULL) {
		return -1;
	}

	assert(OBJ_RC(obj) > 0);

	if (OBJ_RC(obj) == 1) {
		tree->cleanup(OBJ_DATA(obj));
		rv = cx_radix_tree_delete(&tree->values, key);
	} else {
		OBJ_RC(obj)--;
	}

	return rv;
}

void *
cx_rc_radix_tree_retain(struct CxRcRadixTree *tree, uint64_t key) {
	rc_t *obj = radix_tree_retain(tree, key);
	if (obj == NULL) {
		return NULL;
	}
	return OBJ_DATA(obj);
}

int
cx_rc_radix_tree_cleanup(struct CxRcRadixTree *tree) {
	cx_radix_tree_cleanup(&tree->values);
	return 0;
}

static void *
lru_rc_radix_tree_retain(void *backend, uint64_t index) {
	return cx_rc_radix_tree_retain(backend, index);
}

static void
lru_rc_radix_tree_retain_value(void *backend, void *value) {
	(void)backend;
	rc_t *obj = ((rc_t *)value) - 1;
	OBJ_RC(obj)++;
}

static int
lru_rc_radix_tree_release(void *backend, uint64_t index) {
	return cx_rc_radix_tree_release(backend, index);
}

const struct CxLruBackendImpl cx_lru_rc_radix_tree = {
		.retain = lru_rc_radix_tree_retain,
		.retain_value = lru_rc_radix_tree_retain_value,
		.release = lru_rc_radix_tree_release,
};
