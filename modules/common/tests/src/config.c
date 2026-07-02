#include <dttr_config.h>
#include <dttr_test_support.h>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

static void assert_ok(DTTR_Result result) {
	assert_int_equal(result.status, DTTR_OK);
}

static void assert_status(DTTR_Result result, DTTR_Status status) {
	assert_int_equal(result.status, status);
}

static bool temp_config_path(char *out, size_t out_size) {
	char dir[MAX_PATH];
	char path[MAX_PATH];

	if (!GetTempPathA((DWORD)sizeof(dir), dir)
		|| !GetTempFileNameA(dir, "dttr", 0, path)) {
		return false;
	}

	if (strlen(path) >= out_size) {
		DeleteFileA(path);
		return false;
	}

	strcpy(out, path);
	return true;
}

static void control_bindings_round_trip(void **) {
	const int confirm = DTTR_Config_ControlActionIndex("confirm");
	const int start_pause = DTTR_Config_ControlActionIndex("start_pause");
	const int menu_confirm = DTTR_Config_ControlActionIndex("menu_confirm");
	assert_true(confirm >= 0);
	assert_true(start_pause >= 0);
	assert_true(menu_confirm >= 0);
	assert_int_equal(DTTR_Config_ControlActionIndex("menu_cancel"), -1);

	DTTR_Config cfg;
	DTTR_Config_SetDefaults(&cfg);

	cfg.control_bindings[confirm] = DTTR_CONFIG_CONTROL_CODE_SCANCODE_BASE + 44;
	cfg.control_bindings[start_pause] = DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_BASE
										+ 6;
	cfg.control_bindings[menu_confirm] = DTTR_CONFIG_CONTROL_CODE_SCANCODE_BASE
										 + SDL_SCANCODE_BACKSPACE;

	DTTR_Config defaults;
	DTTR_Config_SetDefaults(&defaults);
	assert_true(DTTR_Config_ControlBindingsChanged(&cfg, &defaults));

	char path[MAX_PATH];
	assert_true(temp_config_path(path, sizeof(path)));
	assert_true(DTTR_Config_Save(path, &cfg));
	assert_true(DTTR_Config_Load(path));
	DeleteFileA(path);

	assert_int_equal(
		dttr_config.control_bindings[confirm],
		DTTR_CONFIG_CONTROL_BINDING_NONE
	);
	assert_int_equal(
		dttr_config.control_bindings[start_pause],
		DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_BASE + 6
	);
	assert_int_equal(
		dttr_config.control_bindings[menu_confirm],
		DTTR_CONFIG_CONTROL_CODE_SCANCODE_BASE + SDL_SCANCODE_BACKSPACE
	);
}

static void mod_configs_round_trip_typed_values(void **) {
	DTTR_Config cfg;
	DTTR_Config_SetDefaults(&cfg);

	assert_ok(DTTR_Config_SetModInt(&cfg, "example.mod", "limit", 4));
	assert_ok(DTTR_Config_SetModFloat(&cfg, "example.mod", "scale", 2.25f));
	const char *tricky = "line1\n\t\"quoted\" \\back\\ /slash";
	assert_ok(DTTR_Config_SetModString(&cfg, "example.mod", "label", tricky));

	DTTR_Config defaults;
	DTTR_Config_SetDefaults(&defaults);
	assert_true(DTTR_Config_ModFieldChanged(&cfg, &defaults, "example.mod", "limit"));

	char path[MAX_PATH];
	assert_true(temp_config_path(path, sizeof(path)));
	assert_true(DTTR_Config_Save(path, &cfg));
	assert_true(DTTR_Config_Load(path));
	DeleteFileA(path);

	float scale = 0.0f;
	char label[64] = {0};
	assert_ok(DTTR_Config_GetModFloat(&dttr_config, "example.mod", "scale", &scale));
	assert_float_equal(scale, 2.25f, 0.0001f);
	assert_ok(DTTR_Config_GetModString(
		&dttr_config,
		"example.mod",
		"label",
		label,
		sizeof(label)
	));
	assert_string_equal(label, tricky);
}

static void mod_configs_validation(void **) {
	DTTR_Config cfg;
	DTTR_Config_SetDefaults(&cfg);

	char too_long[DTTR_CONFIG_MOD_STRING_MAX + 1];
	memset(too_long, 'a', sizeof(too_long) - 1);
	too_long[sizeof(too_long) - 1] = '\0';
	assert_status(
		DTTR_Config_SetModString(&cfg, "example.mod", "name", too_long),
		DTTR_ERR_INVALID_ARGUMENT
	);
	assert_status(
		DTTR_Config_SetModFloat(&cfg, "example.mod", "scale", (float)NAN),
		DTTR_ERR_INVALID_ARGUMENT
	);
	assert_status(
		DTTR_Config_SetModBool(&cfg, "example.mod", "", true),
		DTTR_ERR_INVALID_ARGUMENT
	);

	for (int i = 0; i < DTTR_CONFIG_MOD_VALUES_MAX; i++) {
		char field_id[32];
		snprintf(field_id, sizeof(field_id), "field%d", i);
		assert_ok(DTTR_Config_SetModInt(&cfg, "example.mod", field_id, i));
	}

	char overflow[32];
	snprintf(overflow, sizeof(overflow), "field%d", DTTR_CONFIG_MOD_VALUES_MAX);
	assert_status(
		DTTR_Config_SetModInt(&cfg, "example.mod", overflow, 0),
		DTTR_ERR_OUT_OF_MEMORY
	);

	int stale = 123;
	assert_status(
		DTTR_Config_GetModInt(&cfg, "example.mod", "missing", &stale),
		DTTR_ERR_NOT_FOUND
	);
	assert_int_equal(stale, 123);
	assert_status(
		DTTR_Config_GetModBool(&cfg, "example.mod", "field0", NULL),
		DTTR_ERR_INVALID_ARGUMENT
	);
	bool bool_value = true;
	assert_status(
		DTTR_Config_GetModBool(&cfg, "example.mod", "field0", &bool_value),
		DTTR_ERR_UNSUPPORTED_CONTRACT
	);
	assert_true(bool_value);
}

static void mod_configs_apply_default(void **) {
	DTTR_Config cfg;
	DTTR_Config_SetDefaults(&cfg);
	assert_ok(DTTR_Config_SetModInt(&cfg, "example.mod", "count", 7));

	const DTTR_ConfigModDefault int_def = {
		.value_type = DTTR_CONFIG_MOD_VALUE_INT,
		.int_value = 42,
	};

	int count = 0;
	assert_ok(
		DTTR_Config_ApplyModFieldDefault(&cfg, "example.mod", "count", &int_def, false)
	);
	assert_ok(DTTR_Config_GetModInt(&cfg, "example.mod", "count", &count));
	assert_int_equal(count, 7);
	assert_ok(
		DTTR_Config_ApplyModFieldDefault(&cfg, "example.mod", "count", &int_def, true)
	);
	assert_ok(DTTR_Config_GetModInt(&cfg, "example.mod", "count", &count));
	assert_int_equal(count, 42);

	const DTTR_ConfigModDefault null_string_def = {
		.value_type = DTTR_CONFIG_MOD_VALUE_STRING,
		.string_value = NULL,
	};
	char caption[16] = "stale";

	assert_ok(DTTR_Config_ApplyModFieldDefault(
		&cfg,
		"example.mod",
		"caption",
		&null_string_def,
		false
	));

	assert_ok(
		DTTR_Config_GetModString(&cfg, "example.mod", "caption", caption, sizeof(caption))
	);

	assert_string_equal(caption, "");

	assert_status(
		DTTR_Config_ApplyModFieldDefault(&cfg, "example.mod", "count", NULL, false),
		DTTR_ERR_INVALID_ARGUMENT
	);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"control-bindings-round-trip", control_bindings_round_trip},
	{"mod-round-trip", mod_configs_round_trip_typed_values},
	{"mod-validation", mod_configs_validation},
	{"mod-apply-default", mod_configs_apply_default},
};

DTTR_TEST_MAIN(TEST_CASES)
