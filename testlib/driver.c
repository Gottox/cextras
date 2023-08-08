/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : driver
 * @created     : Tuesday Jul 25, 2023 13:20:23 CEST
 */

#define _XOPEN_SOURCE

#include <fnmatch.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "include/testlib.h"

extern const struct TestlibTest testlib_tests[];

static char program_name[4096] = {0};
static const char *color_bad = "";
static const char *color_status = "";
static const char *color_good = "";
static const char *color_reset = "";
static char **patterns = NULL;
static int pattern_count = 0;

static void
color_on(char *opt) {
	bool enable;

	if (strcmp("always", opt) == 0) {
		enable = true;
	} else if (strcmp("never", opt) == 0) {
		enable = false;
	} else if (strcmp("auto", opt) == 0) {
		enable = isatty(STDERR_FILENO);
	} else {
		fprintf(stderr, "%s: invalid argument '%s' for -c\n", program_name,
				opt);
		exit(EXIT_FAILURE);
	}

	if (enable) {
		color_bad = "\x1b[31;1m";
		color_status = "\x1b[33m";
		color_good = "\x1b[32;1m";
		color_reset = "\x1b[0m";
	} else {
		color_bad = "";
		color_status = "";
		color_good = "";
		color_reset = "";
	}
}

static void
run_test(const struct TestlibTest *test) {
	bool found = true;
	for (int i = 0; i < pattern_count; i++) {
		found = false;
		if (fnmatch(patterns[i], test->name, 0) == 0) {
			found = true;
			break;
		}
	}

	if (found == false) {
		fprintf(stderr, "%s '%s'\n IGNORED\n", program_name, test->name);
	} else if (test->enabled == false) {
		fprintf(stderr, "%s '%s'\n DISABLED\n", program_name, test->name);
	} else {
		clock_t time = clock();
		fprintf(stderr, "%s%s '%s'%s\n", color_reset, program_name, test->name,
				color_status);
		test->func();
		fprintf(stderr, "%s finished in %lfms\n", color_reset,
				(double)(clock() - time) * 1000.0 / (double)CLOCKS_PER_SEC);
	}
}

int
main(int argc, char *argv[]) {
	strncpy(program_name, argv[0], sizeof(program_name) - 1);
	int opt;

	color_on("auto");
	while ((opt = getopt(argc, argv, "lc:")) != -1) {
		switch (opt) {
		case 'c':
			color_on(optarg);
			break;
		case 'l':
			for (int i = 0; testlib_tests[i].name != NULL; i++) {
				printf("%s\n", testlib_tests[i].name);
			}
			exit(EXIT_SUCCESS);
		default:
			fprintf(stderr, "Usage: %s [-c always|never|auto] [-l] [test...]\n",
					program_name);
			exit(EXIT_FAILURE);
		}
	}

	patterns = &argv[optind];
	pattern_count = argc - optind;

	for (int i = 0; testlib_tests[i].name != NULL; i++) {
		const struct TestlibTest *test = &testlib_tests[i];

		run_test(test);
	}

	return 0;
}
