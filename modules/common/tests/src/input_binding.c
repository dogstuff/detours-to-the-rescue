#include <dttr_input_binding.h>

#include <dttr_test_support.h>

#include <string.h>

static void assert_ok(DTTR_Result result) {
	assert_int_equal(result.status, DTTR_OK);
}

static void assert_status(DTTR_Result result, DTTR_Status status) {
	assert_int_equal(result.status, status);
}

static void input_binding_round_trip(void **state) {
	const char *tokens[] = {"key:f5", "mouse:right", "pad:a"};
	for (size_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); i++) {
		DTTR_Mods_ConfigInputBinding binding = {0};
		assert_ok(DTTR_InputBinding_Parse(tokens[i], &binding));

		char token[64] = "stale";
		assert_ok(DTTR_InputBinding_Format(&binding, token, sizeof(token)));
		assert_string_equal(token, tokens[i]);
	}

	DTTR_Mods_ConfigInputBinding none = {0};
	assert_ok(DTTR_InputBinding_Parse("", &none));
	assert_int_equal(none.device, DTTR_MODS_BINDING_NONE);

	char token[64] = "stale";
	assert_ok(DTTR_InputBinding_Format(&none, token, sizeof(token)));
	assert_string_equal(token, "");

	char display[32] = "stale";
	assert_ok(DTTR_InputBinding_DisplayName(&none, display, sizeof(display)));
	assert_string_equal(display, "Unbound");
}

static void input_binding_rejects_junk(void **state) {
	const char *junk[] = {"joystick:up", "key:not_a_real_key", "mouse:nope", NULL};
	for (size_t i = 0; i < sizeof(junk) / sizeof(junk[0]); i++) {
		DTTR_Mods_ConfigInputBinding binding = {0};
		assert_ok(DTTR_InputBinding_Parse(junk[i], &binding));
		assert_int_equal(binding.device, DTTR_MODS_BINDING_NONE);
	}

	DTTR_Mods_ConfigInputBinding binding = {0};
	char token[64] = "stale";
	assert_status(DTTR_InputBinding_Parse("key:f5", NULL), DTTR_ERR_INVALID_ARGUMENT);
	assert_status(DTTR_InputBinding_Format(&binding, token, 0), DTTR_ERR_INVALID_ARGUMENT);
	assert_status(
		DTTR_InputBinding_DisplayName(&binding, token, 0),
		DTTR_ERR_INVALID_ARGUMENT
	);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"input-binding-round-trip", input_binding_round_trip},
	{"input-binding-rejects-junk", input_binding_rejects_junk},
};

DTTR_TEST_MAIN(TEST_CASES)
