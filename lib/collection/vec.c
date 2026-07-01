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

#define TO_PTR(v, i) (&(v)->data[(i) * (v)->element_size])

#define CX_VEC_DEFAULT_CAPACITY 16

void
cx_vec_init(struct CxVec *vec, size_t element_size) {
	memset(vec, 0, sizeof(*vec));
	vec->element_size = element_size;
}

int
cx_vec_reserve(struct CxVec *vec, size_t capacity) {
	if (capacity <= vec->capacity) {
		return 0;
	}
	size_t alloc_size;
	if (CX_MUL_OVERFLOW(capacity, vec->element_size, &alloc_size)) {
		return -1;
	}
	void *new_data = realloc(vec->data, alloc_size);
	if (new_data == NULL) {
		return -1;
	}
	vec->data = new_data;
	vec->capacity = capacity;
	return 0;
}

void *
cx_vec_push(struct CxVec *vec, void *value) {
	if (vec->size == vec->capacity) {
		size_t new_capacity;
		if (vec->capacity == 0) {
			new_capacity = CX_VEC_DEFAULT_CAPACITY;
		} else if (CX_MUL_OVERFLOW(vec->capacity, 2, &new_capacity)) {
			return NULL;
		}
		if (cx_vec_reserve(vec, new_capacity) < 0) {
			return NULL;
		}
	}
	/* No overflow check needed. We have an upper bound from capacity. */
	const size_t new_size = vec->size + 1;
	void *ptr = TO_PTR(vec, vec->size);
	memcpy(ptr, value, vec->element_size);
	vec->size = new_size;
	return ptr;
}

void *
cx_vec_pull(struct CxVec *vec, void *value) {
	void *ptr = cx_vec_peek(vec);
	if (ptr == NULL) {
		return NULL;
	}
	memcpy(value, ptr, vec->element_size);
	vec->size--;
	return value;
}

void *
cx_vec_get(const struct CxVec *vec, size_t index) {
	if (index >= vec->size) {
		return NULL;
	}
	return TO_PTR(vec, index);
}

void *
cx_vec_peek(const struct CxVec *vec) {
	if (vec->size == 0) {
		return NULL;
	}
	return TO_PTR(vec, vec->size - 1);
}

size_t
cx_vec_size(const struct CxVec *vec) {
	return vec->size;
}

void
cx_vec_cleanup(struct CxVec *tree) {
	free(tree->data);
	memset(tree, 0, sizeof(struct CxVec));
}
