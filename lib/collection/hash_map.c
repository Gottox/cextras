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

struct CxHashSlot {
	uint64_t key;
	uint8_t state;
	uint8_t psl;
	// void data[];
};

static size_t
slot_size(size_t element_size) {
	size_t base = sizeof(struct CxHashSlot);
	// Align element_size to void* boundary for safe access
	size_t aligned =
			(element_size + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
	return base + aligned;
}

static struct CxHashSlot *
get_slot(void *slots, size_t element_size, size_t index) {
	char *base = (char *)slots;
	return (struct CxHashSlot *)(base + index * slot_size(element_size));
}

static void *
get_data(struct CxHashSlot *slot) {
	return &slot[1];
}

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
hash_map_needs_resize(struct CxHashMap *hash_map) {
	return hash_map->count * LOAD_DENOM >= hash_map->capacity * LOAD_NUMER;
}

static struct CxHashSlot *
key_to_slot(const struct CxHashMap *hash_map, uint64_t key) {
	if (hash_map->capacity == 0) {
		return NULL;
	}

	size_t index = key_to_index(key, hash_map->capacity);
	uint8_t psl = 0;

	while (psl <= MAX_PSL) {
		struct CxHashSlot *slot =
				get_slot(hash_map->slots, hash_map->element_size, index);

		if (slot->state == SLOT_EMPTY) {
			return NULL;
		}

		if (slot->state == SLOT_OCCUPIED && slot->key == key) {
			return slot;
		}

		psl++;
		index = (index + 1) % hash_map->capacity;
	}

	return NULL;
}

static void
robin_hood_insert_slot(
		void *slots, size_t capacity, size_t element_size,
		const struct CxHashSlot *slot) {
	size_t full_size = slot_size(element_size);
	char cur[full_size];
	memcpy(cur, slot, full_size);
	struct CxHashSlot *cur_slot = (struct CxHashSlot *)cur;
	size_t index = key_to_index(cur_slot->key, capacity);
	uint8_t psl = 0;

	while (psl <= MAX_PSL) {
		struct CxHashSlot *target = get_slot(slots, element_size, index);
		char tmp[full_size];
		memcpy(tmp, target, full_size);
		struct CxHashSlot *tmp_slot = (struct CxHashSlot *)tmp;

		if (tmp_slot->state == SLOT_EMPTY ||
			tmp_slot->state == SLOT_TOMBSTONE) {
			cur_slot->psl = psl;
			memcpy(target, cur, full_size);
			return;
		}

		if (tmp_slot->psl < psl) {
			cur_slot->psl = psl;
			memcpy(target, cur, full_size);
			memcpy(cur, tmp, full_size);
			psl = cur_slot->psl;
		}

		psl++;
		index = (index + 1) % capacity;
	}
}

static int
hash_map_grow(struct CxHashMap *hash_map) {
	size_t new_capacity;
	void *new_slots;
	void *old_slots = hash_map->slots;
	size_t old_capacity = hash_map->capacity;
	size_t element_size = hash_map->element_size;

	if (CX_MUL_OVERFLOW(hash_map->capacity, 2, &new_capacity)) {
		return -CX_ERR_INTEGER_OVERFLOW;
	}

	new_slots = calloc(new_capacity, slot_size(element_size));
	if (new_slots == NULL) {
		return -CX_ERR_ALLOC;
	}

	for (size_t i = 0; i < old_capacity; i++) {
		struct CxHashSlot *old_slot = get_slot(old_slots, element_size, i);
		if (old_slot->state == SLOT_OCCUPIED) {
			robin_hood_insert_slot(
					new_slots, new_capacity, element_size, old_slot);
		}
	}

	free(old_slots);
	hash_map->slots = new_slots;
	hash_map->capacity = new_capacity;

	return 0;
}

static void *
hash_map_insert(struct CxHashMap *hash_map, uint64_t key, void *data) {
	size_t index = key_to_index(key, hash_map->capacity);
	size_t element_size = hash_map->element_size;
	size_t full_size = slot_size(element_size);

	char cur_buf[full_size];
	memset(cur_buf, 0, full_size);
	struct CxHashSlot *cur = (struct CxHashSlot *)cur_buf;
	cur->key = key;
	cur->state = SLOT_OCCUPIED;
	cur->psl = 0;
	memcpy(get_data(cur), data, element_size);

	uint8_t psl = 0;
	void *result = NULL;

	while (psl <= MAX_PSL) {
		struct CxHashSlot *slot =
				get_slot(hash_map->slots, element_size, index);

		if (slot->state == SLOT_EMPTY || slot->state == SLOT_TOMBSTONE) {
			cur->psl = psl;
			memcpy(slot, cur, full_size);
			hash_map->count++;
			if (result == NULL) {
				result = get_data(slot);
			}
			return result;
		}

		if (slot->state == SLOT_OCCUPIED && slot->key == cur->key) {
			return get_data(slot);
		}

		if (slot->psl < psl) {
			cur->psl = psl;
			char tmp_buf[full_size];
			memcpy(tmp_buf, slot, full_size);
			memcpy(slot, cur, full_size);
			if (result == NULL) {
				result = get_data(slot);
			}
			memcpy(cur, tmp_buf, full_size);
			psl = cur->psl;
		}

		psl++;
		index = (index + 1) % hash_map->capacity;
	}

	return NULL;
}

int
cx_hash_map_init(struct CxHashMap *hash_map, size_t size, size_t element_size) {
	memset(hash_map, 0, sizeof(*hash_map));

	hash_map->slots = calloc(size, slot_size(element_size));
	if (hash_map->slots == NULL) {
		return -CX_ERR_ALLOC;
	}

	hash_map->capacity = size;
	hash_map->count = 0;
	hash_map->element_size = element_size;

	return 0;
}

void *
cx_hash_map_put(struct CxHashMap *hash_map, uint64_t key, void *data) {
	int rv;
	struct CxHashSlot *slot = key_to_slot(hash_map, key);

	if (slot != NULL) {
		return get_data(slot);
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
cx_hash_map_size(const struct CxHashMap *hash_map) {
	return hash_map->capacity;
}

void *
cx_hash_map_get(const struct CxHashMap *hash_map, uint64_t key) {
	struct CxHashSlot *slot = key_to_slot(hash_map, key);
	if (slot == NULL) {
		return NULL;
	}

	return get_data(slot);
}

int
cx_hash_map_delete(struct CxHashMap *hash_map, uint64_t key) {
	struct CxHashSlot *slot = key_to_slot(hash_map, key);
	if (slot == NULL) {
		return 0;
	}

	slot->state = SLOT_TOMBSTONE;
	hash_map->count--;

	return 0;
}

int
cx_hash_map_cleanup(struct CxHashMap *hash_map) {
	free(hash_map->slots);
	hash_map->slots = NULL;
	hash_map->capacity = 0;
	hash_map->count = 0;

	return 0;
}
