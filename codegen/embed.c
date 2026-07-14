#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[]) {
	assert(argc == 5);
	char *input_path = argv[1];
	char *source_path = argv[2];
	char *header_path = argv[3];
	char *varname = argv[4];

	FILE *in = fopen(input_path, "rb");
	if (!in) {
		perror(input_path);
		return 1;
	}

	FILE *out_source = fopen(source_path, "w");
	if (!out_source) {
		perror(source_path);
		return 1;
	}
	FILE *out_header = fopen(header_path, "w");
	if (!out_header) {
		perror(header_path);
		return 1;
	}
	fprintf(out_source, "const unsigned char %s[] = {", varname);
	size_t size = 0;
	for (; feof(in) == 0; size++) {
		fprintf(out_source, "%d,", fgetc(in));
	}
	fputs("};\n", out_source);

	for (size_t i = 0; i < strlen(header_path); i++) {
		if (isalnum(header_path[i])) {
			header_path[i] = toupper(header_path[i]);
		} else {
			header_path[i] = '_';
		}
	}
	fprintf(out_header,
			"#ifndef %s_H\n"
			"#define %s_H\n"
			"#define %s_size ((size_t)%zu)\n"
			"extern const unsigned char %s[];\n"
			"#endif\n",
			header_path, header_path, varname, size, varname);

	fclose(out_header);
	fclose(out_source);
	fclose(in);
}
