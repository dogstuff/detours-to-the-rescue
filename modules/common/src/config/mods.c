#include "config_internal.h"

#include <math.h>

static bool result_failed(DTTR_Result result) {
	return result.status != DTTR_OK;
}

static void set_result(DTTR_Result *out, DTTR_Result result) {
	if (!out) {
		return;
	}

	*out = result;
}

static DTTR_Result mod_string_id_result(
	const char *id,
	size_t max_len,
	const char *required_message,
	const char *too_long_message
) {
	if (!id || !id[0]) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, required_message);
	}

	if (SDL_strlen(id) >= max_len) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, too_long_message);
	}

	return config_ok();
}

static DTTR_Result mod_config_result(const DTTR_Config *config, const char *mod_id) {
	if (!config) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, "Config is required.");
	}

	return mod_string_id_result(
		mod_id,
		DTTR_CONFIG_MOD_ID_MAX,
		"Mod ID is required.",
		"Mod ID exceeds the configured maximum length."
	);
}

static DTTR_Result mod_value_result(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id
) {
	DTTR_Result result = mod_config_result(config, mod_id);
	if (result_failed(result)) {
		return result;
	}

	return mod_string_id_result(
		field_id,
		DTTR_CONFIG_MOD_FIELD_ID_MAX,
		"Mod field ID is required.",
		"Mod field ID exceeds the configured maximum length."
	);
}

static int find_mod_config(const DTTR_Config *config, const char *mod_id) {
	if (!config || !mod_id || !mod_id[0]
		|| SDL_strlen(mod_id) >= DTTR_CONFIG_MOD_ID_MAX) {
		return -1;
	}

	for (int i = 0; i < config->mod_config_count; i++) {
		if (SDL_strcmp(config->mod_configs[i].mod_id, mod_id) == 0) {
			return i;
		}
	}

	return -1;
}

static int ensure_mod_config(DTTR_Config *config, const char *mod_id, DTTR_Result *out) {
	DTTR_Result result = mod_config_result(config, mod_id);
	if (result_failed(result)) {
		set_result(out, result);
		return -1;
	}

	int index = find_mod_config(config, mod_id);
	if (index >= 0) {
		set_result(out, config_ok());
		return index;
	}

	if (config->mod_config_count >= DTTR_CONFIG_MOD_CONFIGS_MAX) {
		set_result(
			out,
			config_result(DTTR_ERR_OUT_OF_MEMORY, "Mod config table is full.")
		);
		return -1;
	}

	index = config->mod_config_count;
	if (!DTTR_Config_StrCopyChecked(
			config->mod_configs[index].mod_id,
			sizeof(config->mod_configs[index].mod_id),
			mod_id
		)) {
		set_result(out, config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod ID is too long."));
		return -1;
	}

	config->mod_configs[index].schema_version = 0;
	config->mod_config_count++;
	set_result(out, config_ok());
	return index;
}

static int find_mod_value_index(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id
) {
	const int mod_index = find_mod_config(config, mod_id);
	if (mod_index < 0 || !field_id || !field_id[0]
		|| SDL_strlen(field_id) >= DTTR_CONFIG_MOD_FIELD_ID_MAX) {
		return -1;
	}

	for (int i = 0; i < config->mod_value_count; i++) {
		const DTTR_ConfigModValue *value = &config->mod_values[i];
		if (value->mod_index == mod_index && SDL_strcmp(value->field_id, field_id) == 0) {
			return i;
		}
	}

	return -1;
}

static DTTR_ConfigModValue *ensure_mod_value(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	DTTR_Result *out
) {
	DTTR_Result result = mod_value_result(config, mod_id, field_id);
	if (result_failed(result)) {
		set_result(out, result);
		return NULL;
	}

	int value_index = find_mod_value_index(config, mod_id, field_id);
	if (value_index >= 0) {
		set_result(out, config_ok());
		return &config->mod_values[value_index];
	}

	if (config->mod_value_count >= DTTR_CONFIG_MOD_VALUES_MAX) {
		set_result(out, config_result(DTTR_ERR_OUT_OF_MEMORY, "Mod value table is full."));
		return NULL;
	}

	const int mod_index = ensure_mod_config(config, mod_id, &result);
	if (mod_index < 0) {
		set_result(out, result);
		return NULL;
	}

	value_index = config->mod_value_count;
	DTTR_ConfigModValue *value = &config->mod_values[value_index];
	SDL_memset(value, 0, sizeof(*value));
	value->mod_index = mod_index;
	if (!DTTR_Config_StrCopyChecked(value->field_id, sizeof(value->field_id), field_id)) {
		set_result(
			out,
			config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod field ID is too long.")
		);
		return NULL;
	}

	config->mod_value_count++;
	set_result(out, config_ok());
	return value;
}

static const DTTR_ConfigModValue *find_mod_value(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id
) {
	const int index = find_mod_value_index(config, mod_id, field_id);
	return index >= 0 ? &config->mod_values[index] : NULL;
}

DTTR_Result DTTR_Config_GetModSchemaVersion(
	const DTTR_Config *config,
	const char *mod_id,
	uint32_t *out_version
) {
	if (!out_version) {
		return config_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Schema version output is required."
		);
	}

	DTTR_Result result = mod_config_result(config, mod_id);
	if (result_failed(result)) {
		return result;
	}

	const int index = find_mod_config(config, mod_id);
	if (index < 0) {
		return config_result(DTTR_ERR_NOT_FOUND, "Mod config was not found.");
	}

	*out_version = config->mod_configs[index].schema_version;
	return config_ok();
}

DTTR_Result DTTR_Config_SetModSchemaVersion(
	DTTR_Config *config,
	const char *mod_id,
	uint32_t schema_version
) {
	DTTR_Result result;
	const int index = ensure_mod_config(config, mod_id, &result);
	if (index < 0) {
		return result;
	}

	config->mod_configs[index].schema_version = schema_version;
	return config_ok();
}

static DTTR_Result get_mod_typed_value(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	DTTR_ConfigModValueType type,
	const DTTR_ConfigModValue **out_value
) {
	if (!out_value) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod value output is required.");
	}

	DTTR_Result result = mod_value_result(config, mod_id, field_id);
	if (result_failed(result)) {
		return result;
	}

	const DTTR_ConfigModValue *value = find_mod_value(config, mod_id, field_id);
	if (!value) {
		return config_result(DTTR_ERR_NOT_FOUND, "Mod config value was not found.");
	}

	if (value->value_type != type) {
		return config_result(
			DTTR_ERR_UNSUPPORTED_CONTRACT,
			"Mod config value has a different type."
		);
	}

	*out_value = value;
	return config_ok();
}

static DTTR_ConfigModValue *set_mod_typed_value(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	DTTR_ConfigModValueType type,
	DTTR_Result *out
) {
	DTTR_ConfigModValue *slot = ensure_mod_value(config, mod_id, field_id, out);
	if (slot) {
		slot->value_type = type;
	}

	return slot;
}

// Bool and int share the same get/set shape. Float promotes ints and string
// guards length, so those stay hand-written below.
#define DTTR_DEFINE_MOD_SCALAR(Name, CType, Kind, member)                                 \
	DTTR_Result DTTR_Config_GetMod##Name(                                                 \
		const DTTR_Config *config,                                                        \
		const char *mod_id,                                                               \
		const char *field_id,                                                             \
		CType *out_value                                                                  \
	) {                                                                                   \
		if (!out_value) {                                                                 \
			return config_result(                                                         \
				DTTR_ERR_INVALID_ARGUMENT,                                                \
				"Mod value output is required."                                           \
			);                                                                            \
		}                                                                                 \
                                                                                          \
		const DTTR_ConfigModValue *value;                                                 \
		DTTR_Result result = get_mod_typed_value(config, mod_id, field_id, Kind, &value); \
		if (result_failed(result)) {                                                      \
			return result;                                                                \
		}                                                                                 \
                                                                                          \
		*out_value = value->member;                                                       \
		return config_ok();                                                               \
	}                                                                                     \
                                                                                          \
	DTTR_Result DTTR_Config_SetMod##Name(                                                 \
		DTTR_Config *config,                                                              \
		const char *mod_id,                                                               \
		const char *field_id,                                                             \
		CType value                                                                       \
	) {                                                                                   \
		DTTR_Result result;                                                               \
		DTTR_ConfigModValue                                                               \
			*slot = set_mod_typed_value(config, mod_id, field_id, Kind, &result);         \
		if (!slot) {                                                                      \
			return result;                                                                \
		}                                                                                 \
                                                                                          \
		slot->member = value;                                                             \
		return config_ok();                                                               \
	}

DTTR_DEFINE_MOD_SCALAR(Bool, bool, DTTR_CONFIG_MOD_VALUE_BOOL, bool_value)
DTTR_DEFINE_MOD_SCALAR(Int, int, DTTR_CONFIG_MOD_VALUE_INT, int_value)

#undef DTTR_DEFINE_MOD_SCALAR

DTTR_Result DTTR_Config_GetModFloat(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	float *out_value
) {
	if (!out_value) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod value output is required.");
	}

	DTTR_Result result = mod_value_result(config, mod_id, field_id);
	if (result_failed(result)) {
		return result;
	}

	const DTTR_ConfigModValue *value = find_mod_value(config, mod_id, field_id);
	if (!value) {
		return config_result(DTTR_ERR_NOT_FOUND, "Mod config value was not found.");
	}

	// JSON serialises whole-number floats as ints (`2` not `2.0`), so accept
	// INT here and promote losslessly.
	switch (value->value_type) {
	case DTTR_CONFIG_MOD_VALUE_FLOAT:
		*out_value = value->float_value;
		return config_ok();
	case DTTR_CONFIG_MOD_VALUE_INT:
		*out_value = (float)value->int_value;
		return config_ok();
	default:
		return config_result(
			DTTR_ERR_UNSUPPORTED_CONTRACT,
			"Mod config value has a different type."
		);
	}
}

DTTR_Result DTTR_Config_SetModFloat(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	float value
) {
	if (!isfinite(value)) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod float value must be finite.");
	}

	DTTR_Result result;
	DTTR_ConfigModValue *slot = set_mod_typed_value(
		config,
		mod_id,
		field_id,
		DTTR_CONFIG_MOD_VALUE_FLOAT,
		&result
	);
	if (!slot) {
		return result;
	}

	slot->float_value = value;
	return config_ok();
}

DTTR_Result DTTR_Config_GetModString(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	char *out_value,
	size_t out_size
) {
	if (!out_value || out_size == 0) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod string output is required.");
	}

	const DTTR_ConfigModValue *value;
	DTTR_Result result = get_mod_typed_value(
		config,
		mod_id,
		field_id,
		DTTR_CONFIG_MOD_VALUE_STRING,
		&value
	);
	if (result_failed(result)) {
		return result;
	}

	if (SDL_strlen(value->string_value) >= out_size) {
		return config_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Mod string output buffer is too small."
		);
	}

	SDL_strlcpy(out_value, value->string_value, out_size);
	return config_ok();
}

DTTR_Result DTTR_Config_SetModString(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	const char *value
) {
	if (!value || SDL_strlen(value) >= DTTR_CONFIG_MOD_STRING_MAX) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod string value is invalid.");
	}

	DTTR_Result result;
	DTTR_ConfigModValue *slot = ensure_mod_value(config, mod_id, field_id, &result);
	if (!slot) {
		return result;
	}

	if (!DTTR_Config_StrCopyChecked(
			slot->string_value,
			sizeof(slot->string_value),
			value
		)) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod string value is too long.");
	}

	slot->value_type = DTTR_CONFIG_MOD_VALUE_STRING;
	return config_ok();
}

DTTR_Result DTTR_Config_ApplyModFieldDefault(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	const DTTR_ConfigModDefault *def,
	bool overwrite
) {
	if (!def) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod field default is required.");
	}

// Scalar defaults share one shape: keep the existing value unless overwriting or
// it is absent. String differs (needs a buffer + NULL fallback), so it stays below.
#define APPLY_SCALAR_DEFAULT(Kind, CType, member, Getter, Setter)                        \
	case Kind: {                                                                         \
		CType existing;                                                                  \
		if (overwrite || result_failed(Getter(config, mod_id, field_id, &existing))) {   \
			return Setter(config, mod_id, field_id, def->member);                        \
		}                                                                                \
		return config_ok();                                                              \
	}

	switch (def->value_type) {
		APPLY_SCALAR_DEFAULT(
			DTTR_CONFIG_MOD_VALUE_BOOL,
			bool,
			bool_value,
			DTTR_Config_GetModBool,
			DTTR_Config_SetModBool
		)
		APPLY_SCALAR_DEFAULT(
			DTTR_CONFIG_MOD_VALUE_INT,
			int,
			int_value,
			DTTR_Config_GetModInt,
			DTTR_Config_SetModInt
		)
		APPLY_SCALAR_DEFAULT(
			DTTR_CONFIG_MOD_VALUE_FLOAT,
			float,
			float_value,
			DTTR_Config_GetModFloat,
			DTTR_Config_SetModFloat
		)
	case DTTR_CONFIG_MOD_VALUE_STRING: {
		char existing[DTTR_CONFIG_MOD_STRING_MAX];
		if (overwrite
			|| result_failed(DTTR_Config_GetModString(
				config,
				mod_id,
				field_id,
				existing,
				sizeof(existing)
			))) {
			return DTTR_Config_SetModString(
				config,
				mod_id,
				field_id,
				def->string_value ? def->string_value : ""
			);
		}

		return config_ok();
	}
	}
#undef APPLY_SCALAR_DEFAULT

	return config_result(
		DTTR_ERR_UNSUPPORTED_CONTRACT,
		"Mod field default uses an unsupported type."
	);
}

static bool mod_values_equal(
	const DTTR_ConfigModValue *lhs,
	const DTTR_ConfigModValue *rhs
) {
	if (!lhs || !rhs || lhs->value_type != rhs->value_type) {
		return false;
	}

	switch (lhs->value_type) {
#define X(Suffix, member, writer, eq)                                                    \
	case DTTR_CONFIG_MOD_VALUE_##Suffix:                                                 \
		return eq(lhs->member, rhs->member);
		DTTR_CONFIG_MOD_VALUE_TYPES(X)
#undef X
	default:
		return false;
	}
}

static const char *mod_id_for_value(
	const DTTR_Config *config,
	const DTTR_ConfigModValue *value
) {
	if (!config || !value || value->mod_index < 0
		|| value->mod_index >= config->mod_config_count) {
		return NULL;
	}

	return config->mod_configs[value->mod_index].mod_id;
}

bool DTTR_Config_ModFieldChanged(
	const DTTR_Config *current,
	const DTTR_Config *base,
	const char *mod_id,
	const char *field_id
) {
	const DTTR_ConfigModValue *current_value = find_mod_value(current, mod_id, field_id);
	const DTTR_ConfigModValue *base_value = find_mod_value(base, mod_id, field_id);

	if (!current_value && !base_value) {
		return false;
	}

	return !mod_values_equal(current_value, base_value);
}

bool DTTR_Config_ModConfigsChanged(const DTTR_Config *current, const DTTR_Config *base) {
	if (!current || !base) {
		return false;
	}

	if (current->mod_config_count != base->mod_config_count
		|| current->mod_value_count != base->mod_value_count) {
		return true;
	}

	for (int i = 0; i < current->mod_config_count; i++) {
		const DTTR_ConfigModConfig *current_mod = &current->mod_configs[i];
		const int base_index = find_mod_config(base, current_mod->mod_id);

		if (base_index < 0
			|| current_mod->schema_version
				   != base->mod_configs[base_index].schema_version) {
			return true;
		}
	}

	for (int i = 0; i < current->mod_value_count; i++) {
		const DTTR_ConfigModValue *current_value = &current->mod_values[i];
		const char *mod_id = mod_id_for_value(current, current_value);
		const DTTR_ConfigModValue *base_value = find_mod_value(
			base,
			mod_id,
			current_value->field_id
		);
		if (!mod_values_equal(current_value, base_value)) {
			return true;
		}
	}

	return false;
}
