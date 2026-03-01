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
 * @file         rc_hash_map.c
 */

#include "../../include/cextras/collection.h"
#include "../../include/cextras/memory.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Internal entry structure stored in the pool.
 * Contains reference count, key, and user data.
 */
struct CxRcEntry {
	atomic_size_t ref_count;
	uint64_t key;
	/* User data follows immediately after this struct */
};

static void *
entry_data(struct CxRcEntry *entry) {
	return &entry[1];
}

static struct CxRcEntry *
data_to_entry(const void *data) {
	return (struct CxRcEntry *)data - 1;
}

int
cx_rc_hash_map_init(
		struct CxRcHashMap *hash_map, size_t size, size_t element_size,
		sqsh_rc_map_cleanup_t cleanup) {
	int rv;

	memset(hash_map, 0, sizeof(*hash_map));

	rv = cx_hash_map_init(&hash_map->map, size, sizeof(struct CxRcEntry *));
	if (rv < 0) {
		return rv;
	}

	size_t pool_size = sizeof(struct CxRcEntry) + element_size;
	cx_prealloc_pool_init2(&hash_map->pool, size, pool_size);

	hash_map->element_size = element_size;
	hash_map->cleanup = cleanup;

	return 0;
}

void *
cx_rc_hash_map_put(struct CxRcHashMap *hash_map, uint64_t key, void *data) {
	/* Check if key already exists */
	struct CxRcEntry **existing =
			cx_hash_map_get(&hash_map->map, key);
	if (existing != NULL) {
		hash_map->cleanup(data);
		atomic_fetch_add(&(*existing)->ref_count, 1);
		return entry_data(*existing);
	}

	/* Allocate new entry from pool */
	struct CxRcEntry *entry = cx_prealloc_pool_get(&hash_map->pool);
	if (entry == NULL) {
		return NULL;
	}

	atomic_init(&entry->ref_count, 1);
	entry->key = key;
	memcpy(entry_data(entry), data, hash_map->element_size);

	/* Insert pointer to entry into hash map */
	struct CxRcEntry **slot = cx_hash_map_put(&hash_map->map, key, &entry);
	if (slot == NULL) {
		cx_prealloc_pool_recycle(&hash_map->pool, entry);
		return NULL;
	}

	return entry_data(entry);
}

size_t
cx_rc_hash_map_size(const struct CxRcHashMap *hash_map) {
	return cx_hash_map_size(&hash_map->map);
}

void *
cx_rc_hash_map_retain(struct CxRcHashMap *hash_map, uint64_t key) {
	struct CxRcEntry **entry_ptr = cx_hash_map_get(&hash_map->map, key);

	if (entry_ptr != NULL) {
		atomic_fetch_add(&(*entry_ptr)->ref_count, 1);
		return entry_data(*entry_ptr);
	}

	return NULL;
}

void
cx_rc_hash_map_retain_value(struct CxRcHashMap *hash_map, const void *value) {
	(void)hash_map;
	struct CxRcEntry *entry = data_to_entry(value);
	atomic_fetch_add(&entry->ref_count, 1);
}

static void
release_entry(struct CxRcHashMap *hash_map, struct CxRcEntry *entry) {
	if (atomic_fetch_sub(&entry->ref_count, 1) == 1) {
		hash_map->cleanup(entry_data(entry));
		cx_hash_map_delete(&hash_map->map, entry->key);
		cx_prealloc_pool_recycle(&hash_map->pool, entry);
	}
}

int
cx_rc_hash_map_release(struct CxRcHashMap *hash_map, const void *element) {
	struct CxRcEntry *entry = data_to_entry(element);
	release_entry(hash_map, entry);
	return 0;
}

int
cx_rc_hash_map_release_key(struct CxRcHashMap *hash_map, uint64_t key) {
	struct CxRcEntry **entry_ptr = cx_hash_map_get(&hash_map->map, key);

	if (entry_ptr != NULL) {
		release_entry(hash_map, *entry_ptr);
	}

	return 0;
}

int
cx_rc_hash_map_cleanup(struct CxRcHashMap *hash_map) {
	cx_hash_map_cleanup(&hash_map->map);
	cx_prealloc_pool_cleanup(&hash_map->pool);

	return 0;
}

static void *
lru_rc_hash_map_retain(void *backend, uint64_t index) {
	return cx_rc_hash_map_retain(backend, index);
}

static void
lru_rc_hash_map_retain_value(void *backend, void *value) {
	cx_rc_hash_map_retain_value(backend, value);
}

static int
lru_rc_hash_map_release(void *backend, uint64_t index) {
	return cx_rc_hash_map_release_key(backend, index);
}

const struct CxLruBackendImpl cx_lru_rc_hash_map = {
		.retain = lru_rc_hash_map_retain,
		.retain_value = lru_rc_hash_map_retain_value,
		.release = lru_rc_hash_map_release,
};
