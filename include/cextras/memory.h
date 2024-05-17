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
 * @file         threadpool.h
 */

#ifndef MEMORY_H

#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "macro.h"
#include <stdbool.h>
#include <stdlib.h>

#define CX_NEW_IMPL(init, type, ...) \
	{ \
		int rv = 0; \
		type *obj = calloc(1, sizeof(type)); \
		if (obj == NULL) { \
			rv = -SQSH_ERROR_MALLOC_FAILED; \
		} else { \
			rv = init(obj, __VA_ARGS__); \
			if (rv < 0) { \
				free(obj); \
				obj = NULL; \
			} \
		} \
		if (err != NULL) { \
			*err = rv; \
		} \
		return obj; \
	}

#define CX_FREE_IMPL(cleanup, obj) \
	{ \
		if (obj == NULL) { \
			return 0; \
		} else { \
			int rv = cleanup(obj); \
			free(obj); \
			return rv; \
		} \
	}

/***************************************
 * memory/rc.c
 */

struct CxRc {
	_Atomic(unsigned int) count;
};

void cx_rc_init(struct CxRc *rc);
void cx_rc_retain(struct CxRc *rc);
CX_NO_UNUSED bool cx_rc_release(struct CxRc *rc);

/***************************************
 * memory/utils.c
 */

CX_NO_UNUSED void *cx_memdup(const void *source, size_t size);

/***************************************
 * memory/prealloc_pool.c
 */

struct CxPreallocPool {
	void **pools;
	size_t element_size;
	size_t next_index;
	void *reuse_pool;
};

void cx_prealloc_pool_init(struct CxPreallocPool *pool, size_t element_size);

void *cx_prealloc_pool_get(struct CxPreallocPool *pool);

void cx_prealloc_pool_recycle(struct CxPreallocPool *pool, void *element);

void cx_prealloc_pool_cleanup(struct CxPreallocPool *pool);

#ifdef __cplusplus
}
#endif
#endif // MEMORY_H
