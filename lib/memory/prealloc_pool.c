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

static char *
reuse_node(struct CxPreallocPool *pool) {
	void **element = pool->reuse_pool;
	void **next = *element;
	pool->reuse_pool = next;

	memset(element, 0, sizeof(void *));
	return (char *)element;
}

static size_t
chunk_capacity(const struct CxPreallocPool *pool) {
	return (pool->element_count * pool->element_size)
			<< (pool->current_chunk - pool->chunks);
}

static char *
add_chunk(struct CxPreallocPool *pool) {
	if (pool->current_chunk == NULL) {
		pool->current_chunk = pool->chunks;
	} else {
		pool->current_chunk += 1;
	}

	if (pool->current_chunk - pool->chunks >= CX_PREALLOC_POOL_MAX_CHUNKS) {
		return NULL;
	}

	pool->next_offset = 0;
	*pool->current_chunk = calloc(chunk_capacity(pool), sizeof(char));
	return *pool->current_chunk;
}

void
cx_prealloc_pool_init2(
		struct CxPreallocPool *pool, size_t element_count,
		size_t element_size) {
	assert(element_count > 0);
	memset(pool, 0, sizeof(struct CxPreallocPool));
	pool->element_count = element_count;
	pool->element_size =
			(element_size + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
}

void *
cx_prealloc_pool_get(struct CxPreallocPool *pool) {
	if (pool->reuse_pool != NULL) {
		return reuse_node(pool);
	}

	pool->next_offset += pool->element_size;
	if (pool->current_chunk == NULL ||
		pool->next_offset >= chunk_capacity(pool)) {
		return add_chunk(pool);
	}

	return *pool->current_chunk + pool->next_offset;
}

void
cx_prealloc_pool_recycle(struct CxPreallocPool *pool, void *element) {
	if (element != NULL) {
		/* Save the next pointer in the element. */
		*(void **)element = pool->reuse_pool;
		pool->reuse_pool = element;
	}
}

void
cx_prealloc_pool_cleanup(struct CxPreallocPool *pool) {
	for (size_t i = 0; i < CX_PREALLOC_POOL_MAX_CHUNKS; i++) {
		free(pool->chunks[i]);
	}
}
#endif

void
cx_prealloc_pool_init(struct CxPreallocPool *pool, size_t element_size) {
	cx_prealloc_pool_init2(pool, 8, element_size);
}
