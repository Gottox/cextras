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
#include "../../include/cextras/error.h"
#include "../../include/cextras/memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOAD_NUMER 3
#define LOAD_DENOM 4
#define MAX_PSL 128

enum SlotState {
	SLOT_EMPTY = 0,
	SLOT_OCCUPIED = 1,
	SLOT_TOMBSTONE = 2,
};

struct CxRcHashSlot {
	uint64_t key;
	size_t ref_count;
	void *data;
	uint8_t state;
	uint8_t psl;
};

static uint64_t
hash_key(uint64_t key) {
	key ^= key >> 33;
	key *= 0xff51afd7ed558ccdULL;
	key ^= key >> 33;
	key *= 0xc4ceb9fe1a85ec53ULL;
	key ^= key >> 33;
	return key;
}

static size_t
key_to_index(uint64_t key, size_t capacity) {
	return hash_key(key) % capacity;
}

static bool
hash_map_needs_resize(struct CxRcHashMap *hash_map) {
	return hash_map->count * LOAD_DENOM >= hash_map->capacity * LOAD_NUMER;
}

static struct CxRcHashSlot *
key_to_slot(struct CxRcHashMap *hash_map, uint64_t key) {
	if (hash_map->capacity == 0) {
		return NULL;
	}

	size_t index = key_to_index(key, hash_map->capacity);
	uint8_t psl = 0;

	while (psl <= MAX_PSL) {
		struct CxRcHashSlot *slot = &hash_map->slots[index];

		if (slot->state == SLOT_EMPTY) {
			return NULL;
		}

		if (slot->state == SLOT_OCCUPIED && slot->key == key) {
			return slot;
		}

		if (slot->state == SLOT_OCCUPIED && slot->psl < psl) {
			return NULL;
		}

		psl++;
		index = (index + 1) % hash_map->capacity;
	}

	return NULL;
}

static struct CxRcHashSlot *
element_to_slot(struct CxRcHashMap *hash_map, const void *element) {
	uint64_t key;

	// We can't be sure that the element is properly aligned, so we use memcpy
	// to avoid undefined behavior.
	memcpy(&key, &((uint8_t *)element)[hash_map->element_size], sizeof(key));

	return key_to_slot(hash_map, key);
}

static void
robin_hood_insert_slot(
		struct CxRcHashSlot *slots, size_t capacity,
		const struct CxRcHashSlot *slot) {
	struct CxRcHashSlot cur;
	memcpy(&cur, slot, sizeof(struct CxRcHashSlot));
	size_t index = key_to_index(cur.key, capacity);
	uint8_t psl = 0;

	while (psl <= MAX_PSL) {
		struct CxRcHashSlot tmp;
		memcpy(&tmp, &slots[index], sizeof(tmp));

		if (tmp.state == SLOT_EMPTY || tmp.state == SLOT_TOMBSTONE) {
			cur.psl = psl;
			memcpy(&slots[index], &cur, sizeof(struct CxRcHashSlot));
			return;
		}

		if (tmp.psl < psl) {
			cur.psl = psl;
			memcpy(&slots[index], &cur, sizeof(struct CxRcHashSlot));
			memcpy(&cur, &tmp, sizeof(struct CxRcHashSlot));
			psl = cur.psl;
		}

		psl++;
		index = (index + 1) % capacity;
	}
}

static int
hash_map_grow(struct CxRcHashMap *hash_map) {
	size_t new_capacity;
	struct CxRcHashSlot *new_slots;
	struct CxRcHashSlot *old_slots = ((struct CxRcHashSlot *)hash_map->slots);
	size_t old_capacity = hash_map->capacity;

	if (CX_MUL_OVERFLOW(hash_map->capacity, 2, &new_capacity)) {
		return -CX_ERR_INTEGER_OVERFLOW;
	}

	new_slots = (struct CxRcHashSlot *)calloc(new_capacity, sizeof(*new_slots));
	if (new_slots == NULL) {
		return -CX_ERR_ALLOC;
	}

	for (size_t i = 0; i < old_capacity; i++) {
		if (old_slots[i].state == SLOT_OCCUPIED) {
			robin_hood_insert_slot(new_slots, new_capacity, &old_slots[i]);
		}
	}

	free(old_slots);
	hash_map->slots = new_slots;
	hash_map->capacity = new_capacity;

	return 0;
}

static void *
hash_map_insert(struct CxRcHashMap *hash_map, uint64_t key, void *data) {
	size_t index = key_to_index(key, hash_map->capacity);

	void *new_data = cx_prealloc_pool_get(&hash_map->pool);
	if (new_data == NULL) {
		return NULL;
	}
	memcpy(new_data, data, hash_map->element_size);
	uint8_t *key_data = &((uint8_t *)new_data)[hash_map->element_size];
	memcpy(key_data, &key, sizeof(key));

	struct CxRcHashSlot cur = {
			.key = key,
			.ref_count = 0,
			.data = new_data,
			.state = SLOT_OCCUPIED,
			.psl = 0,
	};

	void *result = new_data;
	uint8_t psl = 0;

	while (psl <= MAX_PSL) {
		struct CxRcHashSlot *slot = &hash_map->slots[index];

		if (slot->state == SLOT_EMPTY || slot->state == SLOT_TOMBSTONE) {
			cur.psl = psl;
			*slot = cur;
			hash_map->count++;
			return result;
		}

		if (slot->state == SLOT_OCCUPIED && slot->key == cur.key) {
			cx_prealloc_pool_recycle(&hash_map->pool, new_data);
			return slot->data;
		}

		if (slot->psl < psl) {
			cur.psl = psl;
			struct CxRcHashSlot tmp = *slot;
			*slot = cur;
			cur = tmp;
			psl = cur.psl;
		}

		psl++;
		index = (index + 1) % hash_map->capacity;
	}

	cx_prealloc_pool_recycle(&hash_map->pool, new_data);
	return NULL;
}

int
cx_rc_hash_map_init(
		struct CxRcHashMap *hash_map, size_t size, size_t element_size,
		sqsh_rc_map_cleanup_t cleanup) {
	memset(hash_map, 0, sizeof(*hash_map));

	hash_map->slots = calloc(size, sizeof(struct CxRcHashSlot));
	if (hash_map->slots == NULL) {
		return -CX_ERR_ALLOC;
	}

	size_t pool_size = sizeof(uint64_t) + element_size;
	cx_prealloc_pool_init(&hash_map->pool, pool_size);

	hash_map->capacity = size;
	hash_map->count = 0;
	hash_map->element_size = element_size;
	hash_map->cleanup = cleanup;

	return 0;
}

void *
cx_rc_hash_map_put(struct CxRcHashMap *hash_map, uint64_t key, void *data) {
	int rv;
	struct CxRcHashSlot *slot = key_to_slot(hash_map, key);

	if (slot != NULL) {
		hash_map->cleanup(data);
		slot->ref_count++;
		return slot->data;
	}

	if (hash_map_needs_resize(hash_map)) {
		rv = hash_map_grow(hash_map);
		if (rv < 0) {
			return NULL;
		}
	}

	void *result = hash_map_insert(hash_map, key, data);
	if (result == NULL) {
		rv = hash_map_grow(hash_map);
		if (rv < 0) {
			return NULL;
		}
		result = hash_map_insert(hash_map, key, data);
	}

	return result;
}

size_t
cx_rc_hash_map_size(const struct CxRcHashMap *hash_map) {
	return hash_map->capacity;
}

void *
cx_rc_hash_map_retain(struct CxRcHashMap *hash_map, uint64_t key) {
	struct CxRcHashSlot *slot = key_to_slot(hash_map, key);

	if (slot != NULL) {
		slot->ref_count++;
		return slot->data;
	}

	return NULL;
}

static void
release_slot(struct CxRcHashMap *hash_map, struct CxRcHashSlot *slot) {
	if (slot->ref_count == 0) {
		hash_map->cleanup(slot->data);
		cx_prealloc_pool_recycle(&hash_map->pool, slot->data);
		slot->state = SLOT_TOMBSTONE;
		slot->data = NULL;
		hash_map->count--;
	} else {
		slot->ref_count--;
	}
}
int
cx_rc_hash_map_release(struct CxRcHashMap *hash_map, const void *element) {
	struct CxRcHashSlot *slot = element_to_slot(hash_map, element);

	release_slot(hash_map, slot);

	return 0;
}

int
cx_rc_hash_map_release_key(struct CxRcHashMap *hash_map, uint64_t key) {
	struct CxRcHashSlot *slot = key_to_slot(hash_map, key);

	if (slot != NULL) {
		release_slot(hash_map, slot);
	}

	return 0;
}

int
cx_rc_hash_map_cleanup(struct CxRcHashMap *hash_map) {
	free(hash_map->slots);
	cx_prealloc_pool_cleanup(&hash_map->pool);
	hash_map->slots = NULL;
	hash_map->capacity = 0;
	hash_map->count = 0;

	return 0;
}

static void *
lru_rc_hash_map_retain(void *backend, uint64_t index) {
	return cx_rc_hash_map_retain(backend, index);
}

static int
lru_rc_hash_map_release(void *backend, uint64_t index) {
	return cx_rc_hash_map_release_key(backend, index);
}

const struct CxLruBackendImpl cx_lru_rc_hash_map = {
		.retain = lru_rc_hash_map_retain,
		.release = lru_rc_hash_map_release,
};
