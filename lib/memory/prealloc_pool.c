#include "../../include/cextras/memory.h"
#include "../../include/cextras/types.h"
#include <assert.h>
#include <string.h>

#if 0
void
cx_prealloc_pool_init2(
		struct CxPreallocPool *pool, size_t element_count,
		size_t element_size) {
	(void)element_count;
	(void)pool;
	pool->element_size = element_size;
}

void *
cx_prealloc_pool_get(struct CxPreallocPool *pool) {
	return calloc(1, pool->element_size);
}

void
cx_prealloc_pool_recycle(struct CxPreallocPool *pool, void *element) {
	(void)pool;
	free(element);
}

void
cx_prealloc_pool_cleanup(struct CxPreallocPool *pool) {
	(void)pool;
}
#else
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
	pool->chunk_size *= 2;
	pool->pools = realloc(pool->pools, pool->pool_count * sizeof(char *));
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
cx_prealloc_pool_init2(
		struct CxPreallocPool *pool, size_t element_count,
		size_t element_size) {
	assert(element_size >= sizeof(union ReuseList));
	assert(element_count > 0);
	memset(pool, 0, sizeof(struct CxPreallocPool));
	pool->chunk_size = element_count * element_size;
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
#endif

void
cx_prealloc_pool_init(struct CxPreallocPool *pool, size_t element_size) {
	cx_prealloc_pool_init2(pool, 8, element_size);
}
