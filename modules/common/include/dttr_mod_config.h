#ifndef DTTR_MOD_CONFIG_H
#define DTTR_MOD_CONFIG_H

#include <dttr_config.h>
#include <dttr_mods.h>

static inline DTTR_Result dttr_mod_config_result(DTTR_Status status, const char *message) {
	return (DTTR_Result){
		.status = status,
		.message = message,
	};
}

static inline DTTR_Result DTTR_ModConfig_ResolveDefaultValue(
	DTTR_Mods_ConfigFieldType type,
	DTTR_Mods_ConfigDefaultValue value,
	const char *string_default,
	DTTR_ConfigModDefault *out
) {
	if (!out) {
		return dttr_mod_config_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Mod default output is required."
		);
	}

	switch (type) {
	case DTTR_MODS_CONFIG_FIELD_BOOL:
		*out = (DTTR_ConfigModDefault){
			.value_type = DTTR_CONFIG_MOD_VALUE_BOOL,
			.bool_value = value.bool_value,
		};
		return dttr_mod_config_result(DTTR_OK, NULL);
	case DTTR_MODS_CONFIG_FIELD_INT:
		*out = (DTTR_ConfigModDefault){
			.value_type = DTTR_CONFIG_MOD_VALUE_INT,
			.int_value = value.int_value,
		};
		return dttr_mod_config_result(DTTR_OK, NULL);
	case DTTR_MODS_CONFIG_FIELD_FLOAT:
		*out = (DTTR_ConfigModDefault){
			.value_type = DTTR_CONFIG_MOD_VALUE_FLOAT,
			.float_value = value.float_value,
		};
		return dttr_mod_config_result(DTTR_OK, NULL);
	case DTTR_MODS_CONFIG_FIELD_STRING:
	case DTTR_MODS_CONFIG_FIELD_ENUM:
	case DTTR_MODS_CONFIG_FIELD_INPUT_BINDING:
		*out = (DTTR_ConfigModDefault){
			.value_type = DTTR_CONFIG_MOD_VALUE_STRING,
			.string_value = string_default ? string_default : "",
		};
		return dttr_mod_config_result(DTTR_OK, NULL);
	default:
		return dttr_mod_config_result(
			DTTR_ERR_UNSUPPORTED_CONTRACT,
			"Unsupported mod config field type."
		);
	}
}

/// Resolves the effective string default for a string/enum/input-binding field,
/// falling back to the zero value or first enum choice when no explicit default is set.
static inline const char *DTTR_ModConfig_StringDefault(
	const DTTR_Mods_ConfigField *field
) {
	if (!field
		|| (field->type != DTTR_MODS_CONFIG_FIELD_STRING
			&& field->type != DTTR_MODS_CONFIG_FIELD_ENUM
			&& field->type != DTTR_MODS_CONFIG_FIELD_INPUT_BINDING)) {
		return "";
	}

	const char *value = field->default_value.string_value;
	if ((!value || !value[0]) && field->type == DTTR_MODS_CONFIG_FIELD_ENUM
		&& field->choices && field->choice_count > 0) {
		value = field->choices[0].value;
	}

	return value ? value : "";
}

static inline DTTR_Result DTTR_ModConfig_ResolveFieldDefault(
	const DTTR_Mods_ConfigField *field,
	const char *string_default,
	DTTR_ConfigModDefault *out
) {
	if (!field || !DTTR_Mods_ConfigField_ValidScalar(field)) {
		return dttr_mod_config_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Mod config field descriptor is invalid."
		);
	}

	return DTTR_ModConfig_ResolveDefaultValue(
		field->type,
		field->default_value,
		string_default,
		out
	);
}

#endif // DTTR_MOD_CONFIG_H
