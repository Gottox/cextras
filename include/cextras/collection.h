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
 * @file         sqsh_primitive_private.h
 */

#ifndef CEXTRA_COLLECTION_H
#define CEXTRA_COLLECTION_H

#include "macro.h"
#include "types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************
 * primitive/buffer.c
 */

/**
 * @brief The CextraBuffer struct is a buffer for arbitrary data.
 *
 * The buffer takes care about resizing and freeing the memory managed by The
 * buffer.
 */
struct CextraBuffer {
	/**
	 * @privatesection
	 */
	uint8_t *data;
	size_t size;
	size_t capacity;
};

/**
 * @internal
 * @memberof CextraBuffer
 * @brief cextra_buffer_init initializes a CextraBuffer.
 *
 * @param[out] buffer The CextraBuffer to initialize.
 *
 * @return 0 on success, less than 0 on error.
 */
CEXTRA_NO_UNUSED int cextra_buffer_init(struct CextraBuffer *buffer);

/**
 * @internal
 * @memberof CextraBuffer
 * @brief cextra_buffer_add_size tells CextraBuffer to increase the buffer by
 * additional_size
 *
 * Please be aware, that the buffer needs to be allocated by
 * cextra_buffer_add_capacity before. Otherwise the function behavior is
 * undefined.
 *
 * @param[in,out] buffer The CextraBuffer to increase.
 * @param[in] additional_size The additional size to increase the buffer.
 *
 * @return 0 on success, less than 0 on error.
 */
CEXTRA_NO_UNUSED int
cextra_buffer_add_size(struct CextraBuffer *buffer, size_t additional_size);

/**
 * @internal
 * @memberof CextraBuffer
 * @brief cextra_buffer_add_size allocates additional memory for the
 * CextraBuffer and sets additional_buffer to the beginning of the additional
 * memory.
 *
 * After cextra_buffer_add_capacity has been called, the buffer will behave
 * undefined if you query data or size. In order to use the buffer again, you
 * need to call cextra_buffer_add_size again.
 *
 * @param[in,out] buffer The CextraBuffer to free.
 * @param[in] additional_buffer The pointer to the additional memory.
 * @param[in] additional_size The size of the additional memory.
 *
 * @return 0 on success, less than 0 on error.
 */
CEXTRA_NO_UNUSED int cextra_buffer_add_capacity(
		struct CextraBuffer *buffer, uint8_t **additional_buffer,
		size_t additional_size);

/**
 * @internal
 * @memberof CextraBuffer
 * @brief cextra_buffer_append
 *
 * @param[in,out] buffer The CextraBuffer to append to.
 * @param[in] source The data to append.
 * @param[in] source_size The size of the data to append.
 *
 * @return 0 on success, less than 0 on error.
 */
CEXTRA_NO_UNUSED int cextra_buffer_append(
		struct CextraBuffer *buffer, const uint8_t *source,
		const size_t source_size);

/**
 * @internal
 * @memberof CextraBuffer
 * @brief moves the data from buffer source to buffer. The source buffer will be
 * invalid after this operation.
 *
 * @param[in,out] buffer The CextraBuffer to move to.
 * @param[in] source The CextraBuffer to move from.
 *
 * @return 0 on success, less than 0 on error.
 */
CEXTRA_NO_UNUSED int
cextra_buffer_move(struct CextraBuffer *buffer, struct CextraBuffer *source);

/**
 * @internal
 * @memberof CextraBuffer
 * @brief cextra_buffer_data returns the data of the CextraBuffer.
 * @param[in] buffer The CextraBuffer to get the data from.
 * @return a pointer to the data of the CextraBuffer.
 */
const uint8_t *cextra_buffer_data(const struct CextraBuffer *buffer);

/**
 * @internal
 * @memberof CextraBuffer
 * @brief cextra_buffer_size returns the size of the CextraBuffer.
 *
 * @param[in] buffer The CextraBuffer to get the size from.
 *
 * @return the size of the CextraBuffer.
 */
size_t cextra_buffer_size(const struct CextraBuffer *buffer);

/**
 * @internal
 * @memberof CextraBuffer
 * @brief cextra_buffer_cleanup frees the memory managed by the CextraBuffer.
 * @param[in,out] buffer The CextraBuffer to cleanup.
 *
 * @return 0 on success, less than 0 on error.
 */
int cextra_buffer_cleanup(struct CextraBuffer *buffer);

/***************************************
 * primitive/rc_map.c
 */

/**
 * @brief The type of the cleanup callback function.
 */
typedef void (*sqsh_rc_map_cleanup_t)(void *data);

/**
 * @internal
 * @brief The CextraRcMap struct is a reference-counted array.
 */
struct CextraRcMap {
	/**
	 * @privatesection
	 */
	uint8_t *data;
	size_t size;
	size_t element_size;
	int *ref_count;
	sqsh_rc_map_cleanup_t cleanup;
};

/**
 * @internal
 * @memberof CextraRcMap
 * @brief Initializes a reference-counted array.
 *
 * @param array The array to initialize.
 * @param size The size of the array.
 * @param element_size The size of each element.
 * @param cleanup The cleanup function.
 * @return 0 on success, a negative value on error.
 */
CEXTRA_NO_UNUSED int cextra_rc_map_init(
		struct CextraRcMap *array, size_t size, size_t element_size,
		sqsh_rc_map_cleanup_t cleanup);

/**
 * @internal
 * @memberof CextraRcMap
 * @brief Tests if a reference-counted array is empty.
 * @param array The array to test.
 * @param index The index to test.
 * @return True if the array is empty, false otherwise.
 */
bool cextra_rc_map_is_empty(struct CextraRcMap *array, cextra_index_t index);

/**
 * @internal
 * @memberof CextraRcMap
 * @brief Sets a value in a reference-counted array.
 *
 * @param array The array to set the value in.
 * @param index The index to set the value at.
 * @param data The data to set.
 * @return 0 on success, a negative value on error.
 */
const void *
cextra_rc_map_set(struct CextraRcMap *array, cextra_index_t index, void *data);

/**
 * @internal
 * @memberof CextraRcMap
 * @brief Gets the size of a reference-counted array.
 *
 * @param array The array to get the size of.
 * @return The size of the array.
 */
size_t cextra_rc_map_size(const struct CextraRcMap *array);

/**
 * @internal
 * @memberof CextraRcMap
 * @brief Retains the data at a specified index in a reference-counted array.
 *
 * @param array The array containing the data.
 * @param index The index of the data.
 * @return A pointer to the retained data.
 */
const void *
cextra_rc_map_retain(struct CextraRcMap *array, cextra_index_t index);

/**
 * @internal
 * @memberof CextraRcMap
 * @brief Releases the reference to the data at a specified index in a
 * reference-counted array.
 *
 * @param array The array containing the data.
 * @param element The element to release.
 * @return 0 on success, a negative value on error.
 */
int cextra_rc_map_release(struct CextraRcMap *array, const void *element);

/**
 * @internal
 * @memberof CextraRcMap
 * @brief Releases the reference to the data at a specified index in a
 * reference-counted array.
 *
 * @param array The array containing the data.
 * @param index The index of the data to release.
 * @return 0 on success, a negative value on error.
 */
int
cextra_rc_map_release_index(struct CextraRcMap *array, cextra_index_t index);

/**
 * @internal
 * @memberof CextraRcMap
 * @brief Checks if the element is contained in the array.
 *
 * @param array The array containing the data.
 * @param element The element to check.
 * @return True if the element is contained in the array, false otherwise.
 */
bool cextra_rc_map_contains(struct CextraRcMap *array, const void *element);

/**
 * @internal
 * @memberof CextraRcMap
 * @brief Cleans up a reference-counted array.
 *
 * @param array The array to cleanup.
 * @return 0 on success, a negative value on error.
 */
int cextra_rc_map_cleanup(struct CextraRcMap *array);

/**
 * @internal
 * @memberof CextraRcMap
 *
 * @brief An implementation table to use CextraRcMap as a CextraLruBackend.
 */
extern const struct CextraLruBackendImpl cextra_lru_rc_map;

/***************************************
 * primitive/rc_hash_map.c
 */

/**
 * @brief The type of the key for the hash map.
 */
typedef uint64_t sqsh_rc_map_key_t;

struct CextraRcHashMapInner;

/**
 * @brief A reference-counted hash map.
 */
struct CextraRcHashMap {
	/**
	 * @privatesection
	 */
	struct CextraRcHashMapInner *hash_maps;
	size_t hash_map_count;
	size_t map_size;
	size_t element_size;
	sqsh_rc_map_cleanup_t cleanup;
};

/**
 * @internal
 * @memberof CextraRcHashMap
 * @brief Initializes a reference-counted array.
 *
 * @param hash_map The array to initialize.
 * @param size The size of the array.
 * @param element_size The size of each element.
 * @param cleanup The cleanup function.
 * @return 0 on success, a negative value on error.
 */
CEXTRA_NO_UNUSED int cextra_rc_hash_map_init(
		struct CextraRcHashMap *hash_map, size_t size, size_t element_size,
		sqsh_rc_map_cleanup_t cleanup);

/**
 * @internal
 * @memberof CextraRcHashMap
 * @brief Sets a value in a reference-counted array.
 *
 * @param hash_map The hash map to set the value in.
 * @param key The key to set the value at.
 * @param data The data to set.
 * @return 0 on success, a negative value on error.
 */
const void *cextra_rc_hash_map_put(
		struct CextraRcHashMap *hash_map, sqsh_rc_map_key_t key, void *data);

/**
 * @internal
 * @memberof CextraRcHashMap
 * @brief Gets the size of a reference-counted hash map.
 *
 * @param hash_map The hash map to get the size of.
 * @return The size of the hash map.
 */
size_t cextra_rc_hash_map_size(const struct CextraRcHashMap *hash_map);

/**
 * @internal
 * @memberof CextraRcHashMap
 * @brief Retains the data at a specified key in a reference-counted hash map.
 *
 * @param hash_map The hash map containing the data.
 * @param key The key of the data.
 * @return A pointer to the retained data.
 */
const void *cextra_rc_hash_map_retain(
		struct CextraRcHashMap *hash_map, sqsh_rc_map_key_t key);

/**
 * @internal
 * @memberof CextraRcHashMap
 * @brief Releases the reference to the data at a specified index in a
 * reference-counted hash map.
 *
 * @param hash_map The hash map containing the data.
 * @param element The element to release.
 * @return 0 on success, a negative value on error.
 */
int cextra_rc_hash_map_release(
		struct CextraRcHashMap *hash_map, const void *element);

/**
 * @internal
 * @memberof CextraRcHashMap
 * @brief Releases the reference to the data at a specified index in a
 * reference-counted hash map.
 *
 * @param hash_map The hash map containing the data.
 * @param key The key of the data to release.
 * @return 0 on success, a negative value on error.
 */
int cextra_rc_hash_map_release_key(
		struct CextraRcHashMap *hash_map, sqsh_rc_map_key_t key);

/**
 * @internal
 * @memberof CextraRcHashMap
 * @brief Cleans up a reference-counted hash map.
 *
 * @param hash_map The hash map to cleanup.
 * @return 0 on success, a negative value on error.
 */
int cextra_rc_hash_map_cleanup(struct CextraRcHashMap *hash_map);

/**
 * @internal
 * @memberof CextraRcHashMap
 *
 * @brief An implementation table to use CextraRcHashap as a CextraLruBackend.
 */
extern const struct CextraLruBackendImpl cextra_lru_rc_hash_map;

/***************************************
 * primitive/lru.c
 */

/**
 * @internal
 * @brief The connection to the backend data structure. used by the LRU cache.
 */
struct CextraLruBackendImpl {
	/**
	 * @brief Function that is called to retain an element.
	 */
	const void *(*retain)(void *backend, cextra_index_t id);
	/**
	 * @brief Function that is called to release an element.
	 */
	int (*release)(void *backend, cextra_index_t id);
};

/**
 * @internal
 * @brief Adds LRU functionality to a backend data structure.
 */
struct CextraLru {
	/**
	 * @privatesection
	 */
	void *backend;
	const struct CextraLruBackendImpl *impl;
	cextra_index_t *items;
	cextra_index_t ring_index;
	size_t size;
};

/**
 * @internal
 * @memberof CextraLru
 * @brief Initializes an LRU cache.
 *
 * @param lru The LRU cache to initialize.
 * @param size The size of the LRU cache.
 * @param impl The implementation of the backend data structure.
 * @param backend The backend to use for the LRU cache.
 * @return 0 on success, a negative value on error.
 */
CEXTRA_NO_UNUSED int cextra_lru_init(
		struct CextraLru *lru, size_t size,
		const struct CextraLruBackendImpl *impl, void *backend);

/**
 * @internal
 * @memberof CextraLru
 * @brief marks an item as recently used.
 * @param lru The LRU cache to mark the item in.
 * @param id The id of the item to touch
 * @return 0 on success, a negative value on error.
 */
CEXTRA_NO_UNUSED int cextra_lru_touch(struct CextraLru *lru, size_t id);

/**
 * @internal
 * @memberof CextraLru
 * @brief Cleans up an LRU cache.
 *
 * @param lru The LRU cache to cleanup.
 *
 * @return 0 on success, a negative value on error.
 */
int cextra_lru_cleanup(struct CextraLru *lru);

/***************************************
 * primitive/reader.c
 */

/**
 * @internal
 * @brief A buffer that is used to read data from a CextraReader.
 */
struct CextraIteratorImpl {
	/**
	 * @privatesection
	 */
	int (*next)(void *iterator, size_t desired_size);
	int (*skip)(void *iterator, size_t amount, size_t desired_size);
	size_t (*block_size)(const void *iterator);
	const uint8_t *(*data)(const void *iterator);
	size_t (*size)(const void *iterator);
};

/**
 * @internal
 * @brief A buffer that is used to read data from a CextraReader.
 */
struct CextraReader {
	/**
	 * @privatesection
	 */
	const struct CextraIteratorImpl *impl;
	void *iterator;

	cextra_index_t iterator_offset;
	cextra_index_t buffer_offset;
	cextra_index_t data_offset;
	size_t size;
	size_t data_size;
	struct CextraBuffer buffer;
	const uint8_t *data;
};

#ifdef __cplusplus
}
#endif
#endif /* CEXTRA_COLLECTION_H */
