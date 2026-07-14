#define _GNU_SOURCE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DIVIDE_CEIL(x, y) ((x) / (y) + !!((x) % (y)))

struct KeyValue {
	uint64_t key;
	uint64_t val;
};

struct TreeInfo {
	uint64_t radix;
	uint64_t num_entries;
	uint64_t max_key;
	uint64_t max_value;
};

struct Type {
	size_t max_size;
	size_t bitsize;
	const char *name;
	size_t pack;
};

struct Node {
	int index;
	struct Node *parent;
	struct Node *children;
	uint64_t key;
	uint64_t val;
};

static const struct Type TYPES[] = {
		{UINT8_MAX, 1, "uint8_t", 8},    {UINT8_MAX, 2, "uint8_t", 4},
		{UINT8_MAX, 4, "uint8_t", 2},    {UINT8_MAX, 8, "uint8_t", 1},
		{UINT16_MAX, 16, "uint16_t", 1}, {UINT32_MAX, 32, "uint32_t", 1},
		{UINT64_MAX, 64, "uint64_t", 1}, {0},
};

static unsigned long
cx_log2(unsigned long x) {
	const unsigned long clz = (unsigned int)__builtin_clzl(x | 1);
	return sizeof(unsigned long) * 8 - 1 - clz;
}

static int
parse_fields(char *line, const char *delim, struct KeyValue *kv) {
	char *saveptr;
	char *field = strtok_r(line, delim, &saveptr);
	if (!field) {
		return -1;
	}
	kv->key = strtoul(field, NULL, 10);

	field = strtok_r(NULL, delim, &saveptr);
	if (!field) {
		return -1;
	}
	kv->val = strtoul(field, NULL, 10);

	return 0;
}

static int
find_limits(FILE *csv, const char *delim, struct TreeInfo *info) {
	char *line = NULL;
	ssize_t line_len = 0;
	size_t line_capacity = 0;
	int rv = 0;
	while ((line_len = getline(&line, &line_capacity, csv)) != -1) {
		struct KeyValue kv = {0};
		rv = parse_fields(line, delim, &kv);
		if (rv < 0) {
			fprintf(stderr, "Invalid line: %s", line);
			goto out;
		}
		if (kv.key > info->max_key) {
			info->max_key = kv.key;
		}
		if (kv.val > info->max_value) {
			info->max_value = kv.val;
		}
		info->num_entries++;
	}
out:
	free(line);
	return rv;
}

int
gen_flat_array(
		FILE *csv, const char *delim, struct KeyValue *entries,
		struct TreeInfo *info) {
	info->value_bits = cx_log2(info->max_value) + 1;
	info->key_bits = cx_log2(info->max_key) + 1;
	info->child_count = (1 << info->radix);
	info->radix_mask = info->child_count - 1;
	int rv = 0;
	char *line = NULL;
	ssize_t line_len = 0;
	size_t line_capacity = 0;
	for (uint64_t i = 0; i < info->num_entries; i++) {
		line_len = getline(&line, &line_capacity, csv);
		if (line_len == -1) {
			fprintf(stderr, "Unexpected EOF\n");
			return -1;
		}

		rv = parse_fields(line, delim, &entries[i]);
		if (rv < 0) {
			fprintf(stderr, "Invalid line: %s", line);
			goto out;
		}
	}

out:
	free(line);
	return rv;
}

static const struct Type *
find_type(size_t max, bool allow_pack) {
	size_t i;
	for (i = 0; TYPES[i].name; i++) {
		if (TYPES[i].max_size / TYPES[i].pack >= max) {
			if (allow_pack) {
				break;
			} else if (TYPES[i].pack == 1) {
				break;
			}
		}
	}
	if (!TYPES[i].name) {
		fprintf(stderr, "No suitable type found\n");
		return NULL;
	}
	return &TYPES[i];
}

static int
generate_node(
		struct KeyValue *entries, struct TreeInfo *info, struct Node *node,
		uint64_t level) {}

static int
generate_tree(
		struct KeyValue *entries, struct TreeInfo *info, struct Node *root) {
	generate_node(entries, info, root, 0);
	return 0;
}

static int
usage(const char *argv0) {
	printf("Usage: %s [options] <csv> <header> <source>\n", argv0);
	fputs("Options:\n"
		  "  -h  Show this help message and exit\n"
		  "  -d  Set the delimiter (default: ',')\n"
		  "Arguments:\n"
		  "  csv     Path to the CSV file\n"
		  "  header  Path to the header file\n"
		  "  source  Path to the source file\n",
		  stderr);
	return 1;
}
int
main(int argc, char *argv[]) {
	int rv = 0;

	char *delim = ",";
	int opt;
	struct KeyValue *entries = NULL;
	struct TreeInfo info = {0};
	while ((opt = getopt(argc, argv, "hd:")) != -1) {
		switch (opt) {
		case 'd':
			delim = optarg;
			break;
		case 'h':
		default:
			return usage(argv[0]);
			return 1;
		}
	}
	if (argc - optind != 3) {
		return usage(argv[0]);
	}

	FILE *csv = fopen(argv[1], "r");
	if (!csv) {
		perror("fopen");
		goto out;
	}

	// First pass: find limits
	rv = find_limits(csv, delim, &info);
	if (rv < 0) {
		goto out;
	}

	rv = fseek(csv, 0, SEEK_SET);
	if (rv < 0) {
		perror("fseek");
		goto out;
	}

	// Second pass: generate flat array
	entries = calloc(info.num_entries, sizeof(*entries));
	rv = gen_flat_array(csv, delim, entries, &info);

	// Third pass: generate tree
	rv = generate_tree(entries, &info);

out:
	free(entries);
	if (rv < 0) {
		return rv;
	} else {
		return EXIT_SUCCESS;
	}
}
