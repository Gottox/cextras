#define _DEFAULT_SOURCE
#include "../../include/cextras/memory.h"
#include "../../include/cextras/types.h"
#include <stdlib.h>
#include <string.h>

#define CX_PREALLOC_POOL_SIZE 1024

struct RopeNode *
reuse_node(struct CxPreallocPool *pool) {
	void *element = pool->reuse_pool;
	void *next = *(void **)element;
	pool->reuse_pool = next;

	memset(element, 0, sizeof(void *));
	return element;
}

void *
add_chunk(struct CxPreallocPool *pool) {
	size_t outer_size = pool->next_index / CX_PREALLOC_POOL_SIZE + 1;
	pool->pools = reallocarray(pool->pools, outer_size, sizeof(void *));
	if (pool->pools == NULL) {
		return NULL;
	}
	struct RopeNode *new_chunk =
			calloc(CX_PREALLOC_POOL_SIZE, pool->element_size);
	if (new_chunk == NULL) {
		return NULL;
	}
	pool->pools[outer_size - 1] = new_chunk;
	return new_chunk;
}

void
cx_prealloc_pool_init(struct CxPreallocPool *pool, size_t element_size) {
	memset(pool, 0, sizeof(struct CxPreallocPool));
	pool->element_size = element_size;
}

void *
cx_prealloc_pool_get(struct CxPreallocPool *pool) {
	if (pool->reuse_pool != NULL) {
		return reuse_node(pool);
	} else if (pool->next_index % CX_PREALLOC_POOL_SIZE == 0) {
		pool->next_index++;
		return add_chunk(pool);
	} else {
		size_t outer_index = pool->next_index / CX_PREALLOC_POOL_SIZE;
		size_t inner_index = pool->next_index % CX_PREALLOC_POOL_SIZE;
		pool->next_index++;

		// We use `char` here to have a byte pointer.
		char *elements = pool->pools[outer_index];
		void *element = &elements[inner_index * pool->element_size];
		return element;
	}
}

void
cx_prealloc_pool_recycle(struct CxPreallocPool *pool, void *element) {
	if (element != NULL) {
		// Save the next pointer in the element.
		*(void **)element = pool->reuse_pool;
		pool->reuse_pool = element;
	}
}

void
cx_prealloc_pool_cleanup(struct CxPreallocPool *pool) {
	const size_t outer_size =
			CX_DIVIDE_CEIL(pool->next_index, CX_PREALLOC_POOL_SIZE);
	for (cx_index_t i = 0; i < outer_size; i++) {
		free(pool->pools[i]);
	}

	free(pool->pools);
}
