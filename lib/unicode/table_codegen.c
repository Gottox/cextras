#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define HEADER "static const uint8_t %s_len_map[] = {\n"

#define FOOTER "};\n"

int
main(int argc, char *argv[]) {
	bool utf16_table = false;

	if (argc > 1 && strcmp(argv[1], "utf16") == 0) {
		utf16_table = true;
	}

	printf(HEADER, utf16_table ? "utf16" : "utf8");
	for (int i = 0; i < 256; i++) {
		if ((i & 0x80) == 0) {
			putchar('1');
		} else if ((i & 0xE0) == 0xC0) {
			putchar(utf16_table ? '1' : '2');
		} else if ((i & 0xF0) == 0xE0) {
			putchar(utf16_table ? '2' : '3');
		} else if ((i & 0xF8) == 0xF0) {
			putchar(utf16_table ? '2' : '4');
		} else {
			putchar('0');
		}
		fputs((i + 1) % 26 && i != 255 ? ", " : ",\n", stdout);
	}
	fputs(FOOTER, stdout);

	return 0;
}
