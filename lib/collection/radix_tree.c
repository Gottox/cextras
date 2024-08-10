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
 * @file         rc_tree.c
 */

#include "../../include/cextras/collection.h"

#include <stdio.h>
#include <string.h>

#if 0
static void
inner_dump_tree(int index, void *node, uint64_t capacity) {
	if (node == NULL) {
		return;
	}
	int indent = (capacity ? __builtin_clzll(capacity) : 64) / CX_RADIX * 2;
	for (int i = 0; i < indent; i++) {
		putchar(' ');
	}
	if (capacity == 0) {
		printf("%02x LEAF: %p\n", index, node);
	} else {
		struct CxRadixNode *branch = node;
		printf("%02x BRNH: %p\n", index, (void *)branch);
		for (uint64_t i = 0; i < CX_RADIX_SIZE; i++) {
			inner_dump_tree(i, branch->children[i], capacity >> CX_RADIX);
		}
	}
}
static void
dump_tree(struct CxRadixTree *tree) {
	puts("------------------");
	inner_dump_tree(0, tree->root, tree->capacity);

}
#else
#	define dump_tree(...)
#endif

static uint64_t
prepare_key(const struct CxRadixTree *tree, uint64_t key) {
	uint64_t capacity = tree->capacity;
	uint64_t result = 0;
	while (capacity != 0) {
		result <<= CX_RADIX;
		result |= key & CX_RADIX_MASK;
		key >>= CX_RADIX;
		capacity >>= CX_RADIX;
	}
	return result;
}

static int
resize(struct CxRadixTree *tree, uint64_t key) {
	while (tree->capacity < key) {
		void *old_root = tree->root;
		tree->capacity = (tree->capacity << CX_RADIX) | CX_RADIX_MASK;

		struct CxRadixBranch *new_root =
				cx_prealloc_pool_get(&tree->branch_pool);
		if (new_root == NULL) {
			return -1;
		}
		new_root->children[0] = old_root;
		tree->root = new_root;
	}
	return 0;
}

void
cx_radix_tree_init(struct CxRadixTree *tree, size_t element_size) {
	tree->capacity = 0;
	cx_prealloc_pool_init(&tree->branch_pool, sizeof(struct CxRadixBranch));
	cx_prealloc_pool_init(&tree->leaf_pool, element_size);
	tree->root = NULL;
}

void *
cx_radix_tree_get(const struct CxRadixTree *tree, uint64_t key) {
	void *node = tree->root;

	key = prepare_key(tree, key);
	printf("key: %lx\n", key);

	uint64_t remaining_capacity = tree->capacity;
	uint64_t remaining_key = key;

	while (remaining_capacity != 0 && node != NULL) {
		const uint8_t index = remaining_key & CX_RADIX_MASK;
		struct CxRadixBranch *branch = node;
		printf("index: %x\n", index);

		remaining_capacity >>= CX_RADIX;
		remaining_key >>= CX_RADIX;

		node = branch->children[index];
	}
	return node;
}

void *
cx_radix_tree_put(struct CxRadixTree *tree, uint64_t key, const void *value) {
	void *leaf = cx_radix_tree_new_leaf(tree, key);
	if (leaf != NULL) {
		memcpy(leaf, value, tree->leaf_pool.element_size);
	}
	return leaf;
}

void *
cx_radix_tree_new_leaf(struct CxRadixTree *tree, uint64_t key) {
	bool occupied = false;
	if (resize(tree, key) < 0) {
		return NULL;
	}

	key = prepare_key(tree, key);
	printf("key: %lx\n", key);
	dump_tree(tree);

	if (tree->root == NULL) {
		tree->root = cx_prealloc_pool_get(&tree->leaf_pool);
		return tree->root;
	}

	void *node = tree->root;
	uint64_t remaining_capacity = tree->capacity;
	uint64_t remaining_key = key;

	while (remaining_capacity != 0) {
		const uint8_t index = remaining_key & CX_RADIX_MASK;
		struct CxRadixBranch *branch = node;
		printf("index: %x\n", index);
		printf("node: %p\n", node);

		remaining_capacity >>= CX_RADIX;
		remaining_key >>= CX_RADIX;

		occupied = branch->children[index] != NULL;
		if (remaining_capacity == 0) {
			branch->children[index] = cx_prealloc_pool_get(&tree->leaf_pool);
		} else if (occupied == false) {
			branch->children[index] = cx_prealloc_pool_get(&tree->branch_pool);
		}
		node = branch->children[index];
	}

	dump_tree(tree);

	if (occupied) {
		return NULL;
	} else {
		return node;
	}
}

int
cx_radix_tree_delete(struct CxRadixTree *tree, uint64_t key) {
	key = prepare_key(tree, key);
	void **node = &tree->root;
	uint64_t remaining_capacity = tree->capacity;
	uint64_t remaining_key = key;

	while (remaining_capacity != 0 && *node != NULL) {
		const uint8_t index = remaining_key & CX_RADIX_MASK;
		struct CxRadixBranch *branch = *node;

		remaining_capacity >>= CX_RADIX;
		remaining_key >>= CX_RADIX;

		node = &branch->children[index];
	}
	if (remaining_capacity == 0) {
		cx_prealloc_pool_recycle(&tree->leaf_pool, *node);
		*node = NULL;
		return 0;
	} else {
		return -1;
	}
}

void
cx_radix_tree_cleanup(struct CxRadixTree *tree) {
	cx_prealloc_pool_cleanup(&tree->branch_pool);
	cx_prealloc_pool_cleanup(&tree->leaf_pool);
}
