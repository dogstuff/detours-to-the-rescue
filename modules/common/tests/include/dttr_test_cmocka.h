#ifndef DTTR_TEST_CMOCKA_H
#define DTTR_TEST_CMOCKA_H

#include <cmocka.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	const char *name;
	CMUnitTestFunction fn;
} DTTR_TestCase;

#define DTTR_TEST_ARRAY_COUNT(TESTS) (sizeof(TESTS) / sizeof(*(TESTS)))
#define DTTR_TEST_MAIN(TESTS)                                                            \
	int main(int argc, char **argv) {                                                    \
		return dttr_test_run_cases((TESTS), DTTR_TEST_ARRAY_COUNT(TESTS), argc, argv);   \
	}

// Runs one cmocka case with the registry name used by ctest case sharding.
static inline int dttr_test_run_case(const DTTR_TestCase *test_case) {
	if (!test_case) {
		return 2;
	}

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_case->fn),
	};
	return cmocka_run_group_tests_name(test_case->name, tests, NULL, NULL);
}

// Skips fixture-dependent tests when the external binary corpus is not present.
static inline void dttr_test_require_available(bool available) {
	if (available) {
		return;
	}

	const char *required = getenv("DTTR_REQUIRE_PCDOGS_FIXTURES");
	if (required && required[0] && strcmp(required, "0") != 0) {
		fail_msg("required PCDOGS fixtures are unavailable");
	}

	skip();
}

// Looks up the command-line test name used when ctest runs a single cmocka case.
static inline const DTTR_TestCase *dttr_test_find_case(
	const DTTR_TestCase *test_cases,
	size_t test_case_count,
	const char *name
) {
	if (!test_cases || !name) {
		return NULL;
	}

	for (size_t i = 0; i < test_case_count; i++) {
		if (strcmp(name, test_cases[i].name) == 0) {
			return &test_cases[i];
		}
	}

	return NULL;
}

// Dispatches cmocka cases for both direct execution and per-case ctest entries.
static inline int dttr_test_run_cases(
	const DTTR_TestCase *test_cases,
	size_t test_case_count,
	int argc,
	char **argv
) {
	if (!test_cases || !argv || argc < 1) {
		return 2;
	}

	if (argc == 2) {
		const DTTR_TestCase *test_case = dttr_test_find_case(
			test_cases,
			test_case_count,
			argv[1]
		);
		if (test_case) {
			return dttr_test_run_case(test_case);
		}

		fprintf(stderr, "unknown test case: %s\n", argv[1]);
		return 2;
	}

	if (argc != 1) {
		fprintf(stderr, "usage: %s [test-case]\n", argv[0]);
		return 2;
	}

	int status = 0;
	for (size_t i = 0; i < test_case_count; i++) {
		const int test_status = dttr_test_run_case(&test_cases[i]);
		if (test_status != 0) {
			status = test_status;
		}
	}

	return status;
}

#endif // DTTR_TEST_CMOCKA_H
