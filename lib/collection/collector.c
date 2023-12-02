/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : iterator_collector
 * @created     : Tuesday Aug 29, 2023 17:27:35 CEST
 */

#include "../../include/cextras/collection.h"
#include "../../include/cextras/error.h"
#include <stdint.h>
#include <sys/wait.h>

static const uintptr_t nullword = 0;

/**
 *
 */
/**
 * @brief Handles the next element in the collection.
 *
 * This function retrieves the next element using the provided iterator and
 * appends information about the element to the specified lists.
 *
 * @param next          The callback function for obtaining the next element
 * from the iterator.
 * @param iterator      The iterator used to traverse the collection.
 * @param list          The buffer to store offset of the values.
 * @param list_values   The buffer to store values.
 *
 * @retval              1 if a new value was added
 * @retval              0 if the iterator ended
 * @retval              negative if an error occured.
 */
static int
handle_next(
		cx_collector_next_t next, void *iterator, struct CxBuffer *list,
		struct CxBuffer *list_values) {
	int rv;
	const char *value = NULL;
	size_t size = 0;

	rv = next(iterator, &value, &size);
	if (rv < 0) {
		return rv;
	} else if (value == NULL) {
		return 0;
	}

	size_t offset = cx_buffer_size(list_values);
	char *offset_ptr = (void *)offset;
	rv = cx_buffer_append(
			list, (const uint8_t *)&offset_ptr, sizeof(uintptr_t));
	if (rv < 0) {
		return rv;
	}
	rv = cx_buffer_append(list_values, (const uint8_t *)value, size);
	if (rv < 0) {
		return rv;
	}
	rv = cx_buffer_append(
			list_values, (const uint8_t *)&nullword, sizeof(char));
	if (rv < 0) {
		return rv;
	}

	return 1;
}

int
cx_collect(char ***target, cx_collector_next_t next, void *iterator) {
	int rv = 0;
	struct CxBuffer list = {0};
	struct CxBuffer list_values = {0};
	size_t elements = 0;
	char **result = NULL;

	rv = cx_buffer_init(&list);
	if (rv < 0) {
		goto out;
	}
	rv = cx_buffer_init(&list_values);
	if (rv < 0) {
		goto out;
	}

	for (;;) {
		rv = handle_next(next, iterator, &list, &list_values);
		if (rv > 0) {
			elements++;
		} else {
			break;
		}
	}
	if (rv < 0) {
		goto out;
	}

	rv = cx_buffer_append(&list, (const uint8_t *)&nullword, sizeof(char *));
	if (rv < 0) {
		goto out;
	}
	size_t base_size = cx_buffer_size(&list);

	const uint8_t *values_data = cx_buffer_data(&list_values);
	size_t values_size = cx_buffer_size(&list_values);

	rv = cx_buffer_append(&list, values_data, values_size);
	if (rv < 0) {
		goto out;
	}

	result = (char **)cx_buffer_unwrap(&list);
	for (cx_index_t i = 0; i < elements; i++) {
		result[i] += base_size + (uintptr_t)result;
	}
	*target = result;

out:
	cx_buffer_cleanup(&list);
	cx_buffer_cleanup(&list_values);
	return rv;
}
