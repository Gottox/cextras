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
#include <stdio.h>
#include <string.h>

void
cx_radix_tree_init(struct CxRadixTree *map, size_t element_size) {
	memset(map, 0, sizeof(struct CxRadixTree));

	cx_prealloc_pool_init(&map->leaf_pool, element_size);
	cx_prealloc_pool_init(&map->node_pool, sizeof(struct CxRadixNode));

	map->capacity = CX_RADIX_MASK;
	map->root = cx_prealloc_pool_get(&map->node_pool);
}

static int
resize(struct CxRadixTree *map, uint64_t key) {
	while (map->capacity < key) {
		struct CxRadixNode *old_root = map->root;
		map->capacity = (map->capacity << CX_RADIX) | CX_RADIX_MASK;

		if (old_root->occupied == 0) {
			continue;
		}
		struct CxRadixNode *new_root = cx_prealloc_pool_get(&map->node_pool);
		if (new_root == NULL) {
			return -1;
		}
		new_root->children[0] = old_root;
		new_root->occupied = 1;
		map->root = new_root;
	}
	return 0;
}

void *
cx_radix_tree_get(const struct CxRadixTree *map, uint64_t key) {
	if (map->capacity < key) {
		return NULL;
	}

	struct CxRadixNode *node = map->root;
	size_t capacity = map->capacity;
	while (capacity >= CX_RADIX_SIZE) {
		node = node->children[key & CX_RADIX_MASK];
		if (node == NULL) {
			return NULL;
		}
		capacity >>= CX_RADIX;
		key >>= CX_RADIX;
	}
	return node->children[key];
}

void *
cx_radix_tree_put(struct CxRadixTree *map, uint64_t key, const void *value) {
	if (resize(map, key) < 0 || cx_radix_tree_get(map, key) != NULL) {
		return NULL;
	}

	struct CxRadixNode *node = map->root;
	size_t capacity = map->capacity;
	while (capacity >= CX_RADIX_SIZE) {
		size_t index = key & CX_RADIX_MASK;
		if (node->children[index] == NULL) {
			struct CxRadixNode *new_node =
					cx_prealloc_pool_get(&map->node_pool);
			if (new_node == NULL) {
				return NULL;
			}
			memset(new_node, 0, sizeof(*new_node));
			node->children[index] = new_node;
			node->occupied++;
		}
		node = node->children[index];
		capacity >>= CX_RADIX;
		key >>= CX_RADIX;
	}
	assert(node->children[key] == NULL);

	node->occupied++;
	void *leaf = node->children[key] = cx_prealloc_pool_get(&map->leaf_pool);
	memcpy(leaf, value, map->leaf_pool.element_size);
	return leaf;
}

int
cx_radix_tree_delete(struct CxRadixTree *map, uint64_t key) {
	if (map->capacity < key || cx_radix_tree_get(map, key) == NULL) {
		return -1;
	}

	struct CxRadixNode *node = map->root;
	size_t capacity = map->capacity;
	while (capacity >= CX_RADIX_SIZE) {
		size_t index = key & CX_RADIX_MASK;
		if (node->children[index] == NULL) {
			return -1;
		}
		struct CxRadixNode *next_node = node->children[index];
		node->occupied--;
		if (node->occupied == 0) {
			cx_prealloc_pool_recycle(&map->node_pool, node);
		}
		capacity >>= CX_RADIX;
		key >>= CX_RADIX;
		node = next_node;
	}

	node->occupied--;
	cx_prealloc_pool_recycle(&map->leaf_pool, node->children[key]);
	return 0;
}

#if 0
static void
print_tree(struct CxRadixNode *node, size_t capacity, size_t depth) {
	for (size_t i = 0; i < depth; i++) {
		fputs("  ", stdout);
	}
	if (node == NULL) {
		puts("+ NULL");
		return;
	} else if (capacity == 1) {
		puts("+ leaf");
		return;
	}

	puts("+ node");
	for (size_t i = 0; i < CX_RADIX_SIZE; i++) {
		print_tree(node->children[i], capacity >> CX_RADIX, depth + 1);
	}
}
#endif

void
cx_radix_tree_cleanup(struct CxRadixTree *map) {
	assert(map->root->occupied == 0);
	cx_prealloc_pool_cleanup(&map->node_pool);
	cx_prealloc_pool_cleanup(&map->leaf_pool);
}
