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
 * @file         buffer.c
 */

#include "../../include/cextras/collection.h"
#include <stdlib.h>
#include <string.h>

void
cx_pin_vec_init(struct CxPinVec *vec, size_t element_size) {
	cx_vec_init(&vec->vec, sizeof(void *));
	cx_prealloc_pool_init(&vec->pool, element_size);
}

int
cx_pin_vec_reserve(struct CxPinVec *vec, size_t capacity) {
	return cx_vec_reserve(&vec->vec, capacity);
}

void *
cx_pin_vec_push(struct CxPinVec *vec, void *value) {
	void *element = cx_prealloc_pool_get(&vec->pool);
	if (element == NULL) {
		return NULL;
	}

	if (cx_vec_push(&vec->vec, &element) == NULL) {
		cx_prealloc_pool_recycle(&vec->pool, element);
		return NULL;
	}
	memcpy(element, value, vec->pool.element_size);
	return element;
}

int
cx_pin_vec_pull(struct CxPinVec *vec) {
	void *element = NULL;
	if (cx_vec_pull(&vec->vec, &element) == NULL) {
		return -1;
	}

	cx_prealloc_pool_recycle(&vec->pool, element);
	return 0;
}

void *
cx_pin_vec_get(const struct CxPinVec *vec, size_t index) {
	void **ptr = cx_vec_get(&vec->vec, index);
	if (ptr == NULL) {
		return NULL;
	}
	return *ptr;
}

void *
cx_pin_vec_peek(const struct CxPinVec *vec) {
	size_t size = cx_vec_size(&vec->vec);
	if (size == 0) {
		return NULL;
	}

	return cx_pin_vec_get(vec, size - 1);
}

size_t
cx_pin_vec_size(const struct CxPinVec *vec) {
	return cx_vec_size(&vec->vec);
}

void
cx_pin_vec_cleanup(struct CxPinVec *vec) {
	cx_vec_cleanup(&vec->vec);
	cx_prealloc_pool_cleanup(&vec->pool);
}
