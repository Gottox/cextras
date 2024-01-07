#define _GNU_SOURCE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Range {
	uint32_t start;
	uint32_t end;
};

static int
parse_range(char *line, int field_nbr, struct Range *range) {
	int rv = 0;

	line = strtok(line, ";");
	rv = sscanf(line, "%x..%x", &range->start, &range->end);
	if (rv != 2) {
		rv = sscanf(line, "%x", &range->start);
		if (rv != 1) {
			return -1;
		}
		range->end = range->start;
	}

	for (int i = 0; i < field_nbr; i++) {
		line = strtok(NULL, ";#");
	}
	// For EastAsianWidth.txt
	if (strcmp(line, " W  ") == 0) {
		return 0;
	}
	// For UnicodeData.txt
	if (strcmp(line, "Cc") == 0) {
		return 0;
	}
	if (strcmp(line, "Cf") == 0) {
		return 0;
	}
	if (strcmp(line, "Mn") == 0) {
		return 0;
	}
	if (strcmp(line, "Me") == 0) {
		return 0;
	}

	return -1;
}

static int
find_max(FILE *file, int field_nbr) {
	char *line = NULL;
	ssize_t line_len = 0;
	size_t line_capacity = 0;
	size_t codepoint_size = 0;
	while ((line_len = getline(&line, &line_capacity, file)) != -1) {
		struct Range range = {0};
		if (parse_range(line, field_nbr, &range) < 0) {
			continue;
		} else if (range.end > codepoint_size) {
			codepoint_size = range.end;
		} else if (range.start > codepoint_size) {
			codepoint_size = range.start;
		}
	}
	free(line);

	return codepoint_size + 1;
}

static bool *
generate_flat_table(FILE *file, int codepoint_size, int field_nbr) {
	bool *table = calloc(codepoint_size, sizeof(bool));
	char *line = NULL;
	ssize_t line_len = 0;
	size_t line_capacity = 0;
	while ((line_len = getline(&line, &line_capacity, file)) != -1) {
		struct Range range = {0};
		if (parse_range(line, field_nbr, &range) < 0) {
			continue;
		}
		for (uint32_t i = range.start; i <= range.end; i++) {
			table[i] = true;
		}
	}
	free(line);

	return table;
}

#define DIVIDE_CEIL(x, y) ((x) / (y) + !!((x) % (y)))
#define PADDING(x, p) (SQSH_DIVIDE_CEIL(x, p) * p)

static void
print_radix_table(
		FILE *out, bool *table, char *table_name, size_t codepoint_size,
		size_t radix) {
	size_t radix_num = 1 << radix;
	size_t level_1_size = DIVIDE_CEIL(codepoint_size, radix_num);
	size_t level_2_size = radix_num / 8;
	static bool need_null = false;
	static bool need_full = false;

	bool *level_1_all_set_map = calloc(level_1_size, sizeof(bool));
	bool *level_1_all_unset_map = calloc(level_1_size, sizeof(bool));

	fputs("#include <stdint.h>\n", out);
	fputs("#include <stddef.h>\n", out);

	fprintf(out, "const uint8_t %s_tbl[] = {\n", table_name);
	size_t table_index = 0;
	for (size_t i = 0; i < level_1_size; i++) {
		bool all_unset = true;
		bool all_set = true;
		for (size_t j = 0; j < radix_num; j++) {
			if (i * radix_num + j >= codepoint_size) {
				continue;
			} else if (table[i * radix_num + j]) {
				all_unset = false;
			} else {
				all_set = false;
			}
		}
		level_1_all_set_map[i] = all_set;
		level_1_all_unset_map[i] = all_unset;

		need_null |= all_unset;
		need_full |= all_set;

		if (all_set || all_unset) {
			continue;
		}
		fprintf(out, "\n#define TBL_%lu %lu\n", i, table_index);
		table_index++;
		uint8_t current_byte = 0;
		for (size_t j = 0; j < radix_num; j++) {
			if (i * radix_num + j < codepoint_size) {
				current_byte |= table[i * radix_num + j] ? (1 << (j % 8)) : 0;
			}

			if (j % 64 == 0) {
				fputs("\t", out);
			}
			if (j % 8 == 7) {
				fprintf(out, "0x%02x, ", current_byte);
				current_byte = 0;
			}
			if (j % 64 == 63) {
				fputs("\n", out);
			}
		}
	}

	if (need_full) {
		fprintf(out, "\n#define TBL_FULL %lu\n", table_index);
		for (size_t i = 0; i < level_2_size; i++) {
			if (i % 8 == 0) {
				fputs("\t", out);
			}
			fputs("0xff, ", out);
			if (i % 8 == 7) {
				fputs("\n", out);
			}
		}
		table_index++;
	}

	if (need_null) {
		fprintf(out, "\n#define TBL_NULL %lu\n", table_index);
		for (size_t i = 0; i < level_2_size; i++) {
			if (i % 8 == 0) {
				fputs("\t", out);
			}
			fputs("0x00, ", out);
			if (i % 8 == 7) {
				fputs("\n", out);
			}
		}
	}
	fputs("};\n", out);

	fprintf(out, "const uint8_t %s_lookup[%lu] = {\n", table_name,
			level_1_size);
	for (size_t i = 0; i < level_1_size; i++) {
		fprintf(out, "\t[0x%04lx] = ", i);
		if (level_1_all_set_map[i]) {
			fputs("TBL_FULL,\n", out);
		} else if (level_1_all_unset_map[i]) {
			fputs("TBL_NULL,\n", out);
		} else {
			fprintf(out, "TBL_%lu,\n", i);
		}
	}
	fputs("};\n", out);
	fprintf(out, "const size_t %s_size = %lu;\n", table_name, codepoint_size);
	fprintf(out, "const size_t %s_radix = %lu;\n", table_name, radix);

	free(level_1_all_set_map);
	free(level_1_all_unset_map);
}

int
main(int argc, char *argv[]) {
	int rv = 0;

	if (argc != 5) {
		fprintf(stderr, "Usage: %s <TABLE> <TABLE_NAME> <FIELD_NBR> <RADIX>\n",
				argv[0]);
		return 1;
	}

	int radix = atoi(argv[4]);
	int field_nbr = atoi(argv[3]);
	char *table_name = argv[2];
	FILE *file = fopen(argv[1], "r");
	int codepoint_size = find_max(file, field_nbr);

	rv = fseek(file, 0, SEEK_SET);
	if (rv < 0) {
		perror("fseek");
		return 1;
	}

	bool *flat_table = generate_flat_table(file, codepoint_size, field_nbr);

	print_radix_table(stdout, flat_table, table_name, codepoint_size, radix);

	free(flat_table);
	fclose(file);

	return 0;
}
