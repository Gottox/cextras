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

int
cx_vec_init(struct CxVec *vec, size_t element_size, size_t capacity) {
	int rv = 0;
	if (capacity == 0) {
		capacity = 16;
	}
	vec->data = calloc(capacity, element_size);
	if (vec->data == NULL) {
		rv = -1;
		goto out;
	}
	vec->capacity = capacity;
	vec->element_size = element_size;
	vec->size = 0;

out:
	if (rv < 0) {
		cx_vec_cleanup(vec);
	}
	return rv;
}

static void *
to_ptr(struct CxVec *vec, size_t index) {
	return (uint8_t *)vec->data + index * vec->element_size;
}

void *
cx_vec_push(struct CxVec *vec, void *value) {
	/* No overflow check needed. We have an upper bound from capacity. */
	const size_t new_size = vec->size + 1;
	if (new_size > vec->capacity) {
		size_t new_capacity;
		if (CX_MUL_OVERFLOW(vec->capacity, 2, &new_capacity)) {
			return NULL;
		}
		size_t alloc_size;
		if (CX_MUL_OVERFLOW(new_capacity, vec->element_size, &alloc_size)) {
			return NULL;
		}
		void *new_data = realloc(vec->data, alloc_size);
		if (new_data == NULL) {
			return NULL;
		}
		vec->data = new_data;
		vec->capacity = new_capacity;
	}
	void *ptr = to_ptr(vec, vec->size);
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
cx_vec_get(struct CxVec *vec, size_t index) {
	if (index >= vec->size) {
		return NULL;
	}
	return to_ptr(vec, index);
}

void *
cx_vec_peek(struct CxVec *vec) {
	if (vec->size == 0) {
		return NULL;
	}
	return to_ptr(vec, vec->size - 1);
}

void
cx_vec_cleanup(struct CxVec *tree) {
	free(tree->data);
	memset(tree, 0, sizeof(struct CxVec));
}
