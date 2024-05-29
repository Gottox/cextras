#define _DEFAULT_SOURCE
#include "../../include/cextras/memory.h"
#include "../../include/cextras/types.h"
#include <assert.h>
#include <string.h>

union ReuseList {
	union ReuseList *next;
	char element;
};

char *
reuse_node(struct CxPreallocPool *pool) {
	union ReuseList *element = pool->reuse_pool;
	union ReuseList *next = element->next;
	pool->reuse_pool = next;

	memset(element, 0, sizeof(union ReuseList));
	return &element->element;
}

int
add_chunk(struct CxPreallocPool *pool) {
	pool->pool_count++;
	pool->pools = reallocarray(pool->pools, pool->pool_count, sizeof(char *));
	if (pool->pools == NULL) {
		return -1;
	}
	char *new_chunk = calloc(pool->chunk_size, sizeof(char));
	if (new_chunk == NULL) {
		return -1;
	}

	pool->current_pool = new_chunk;
	pool->pools[pool->pool_count - 1] = new_chunk;
	pool->next_offset = 0;
	return 0;
}

void
cx_prealloc_pool_init(struct CxPreallocPool *pool, size_t element_size) {
	cx_prealloc_pool_init2(pool, 4096, element_size);
}

void
cx_prealloc_pool_init2(
		struct CxPreallocPool *pool, size_t chunk_size, size_t element_size) {
	pool->chunk_size = chunk_size;
	assert(element_size >= sizeof(union ReuseList));
	assert(element_size <= pool->chunk_size);
	memset(pool, 0, sizeof(struct CxPreallocPool));
	pool->element_size = element_size;
}

void *
cx_prealloc_pool_get(struct CxPreallocPool *pool) {
	if (pool->reuse_pool != NULL) {
		return reuse_node(pool);
	} else if (
			pool->pools == NULL ||
			pool->next_offset + pool->element_size > pool->chunk_size) {
		if (add_chunk(pool) < 0) {
			return NULL;
		}
	}

	void *element = &pool->current_pool[pool->next_offset];
	pool->next_offset += pool->element_size;
	return element;
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
	for (cx_index_t i = 0; i < pool->pool_count; i++) {
		free(pool->pools[i]);
	}

	free(pool->pools);
}
