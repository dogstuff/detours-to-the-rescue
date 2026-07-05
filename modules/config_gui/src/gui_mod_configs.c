#include "gui_internal.h"
#include "gui_mod_load_path.h"

#include <dttr_mod_config.h>

static bool is_shadow_mod_dll(const char *filename) {
	return filename
		   && strncmp(
				  filename,
				  DTTR_MODS_SHADOW_PREFIX,
				  sizeof(DTTR_MODS_SHADOW_PREFIX) - 1
			  ) == 0;
}

static bool build_mods_dll_glob(const char *mods_dir, char *out, size_t out_size) {
	sds pattern = sdsnew(mods_dir);
	const bool ok = pattern
					&& DTTR_Path_AppendSegment(
						&pattern,
						"*.dll",
						DTTR_PATH_NATIVE_SEPARATOR
					)
					&& DTTR_Path_CopySds(out, out_size, pattern);
	sdsfree(pattern);
	return ok;
}

void for_each_mod_dll(const char *mods_dir, mod_dll_visitor visit, void *user_data) {
	char pattern[MAX_PATH];
	if (!mods_dir || !mods_dir[0]
		|| !build_mods_dll_glob(mods_dir, pattern, sizeof(pattern))) {
		return;
	}

	WIN32_FIND_DATAA find_data;
	HANDLE find_handle = FindFirstFileA(pattern, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE) {
		return;
	}

	do {
		if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			|| is_shadow_mod_dll(find_data.cFileName)) {
			continue;
		}

		if (!visit(find_data.cFileName, user_data)) {
			break;
		}
	} while (FindNextFileA(find_handle, &find_data));

	FindClose(find_handle);
}

static bool copy_ui_string(
	char *out,
	size_t out_size,
	const char *value,
	bool allow_empty
) {
	if (!allow_empty && value && !value[0]) {
		return false;
	}

	return DTTR_Config_StrCopyChecked(out, out_size, value);
}

static bool copy_ui_label_and_tooltip(
	char *label,
	size_t label_size,
	const char *src_label,
	const char *label_fallback,
	char *tooltip,
	size_t tooltip_size,
	const char *src_tooltip
) {
	return copy_ui_string(label, label_size, src_label ? src_label : label_fallback, false)
		   && copy_ui_string(tooltip, tooltip_size, src_tooltip ? src_tooltip : "", true);
}

static bool has_mod_config_field(
	const config_ui_state *state,
	int mod_index,
	const char *id
) {
	for (int i = 0; i < state->mod_config_field_count; i++) {
		const config_mod_field_ui *field = &state->mod_config_fields[i];

		if (field->mod_index == mod_index && strcmp(field->id, id) == 0) {
			return true;
		}
	}

	return false;
}

static bool has_mod_config(const config_ui_state *state, const char *mod_id) {
	for (int i = 0; i < state->mod_config_count; i++) {
		if (strcmp(state->mod_configs[i].mod_id, mod_id) == 0) {
			return true;
		}
	}

	return false;
}

static bool mod_config_field_type_valid(DTTR_Mods_ConfigFieldType type) {
	return type >= DTTR_MODS_CONFIG_FIELD_BOOL
		   && type <= DTTR_MODS_CONFIG_FIELD_INPUT_BINDING;
}

static bool append_mod_config_choice(
	config_ui_state *state,
	const DTTR_Mods_ConfigChoice *source
) {
	if (state->mod_config_choice_count >= DTTR_CONFIG_UI_MOD_CONFIG_CHOICES_MAX || !source
		|| !source->value || !source->value[0]) {
		return false;
	}

	config_mod_choice_ui
		*choice = &state->mod_config_choices[state->mod_config_choice_count];

	if (!copy_ui_string(choice->value, sizeof(choice->value), source->value, false)
		|| !copy_ui_label_and_tooltip(
			choice->label,
			sizeof(choice->label),
			source->label,
			source->value,
			choice->tooltip,
			sizeof(choice->tooltip),
			source->tooltip
		)) {
		return false;
	}

	state->mod_config_choice_count++;
	return true;
}

static bool append_mod_config_field(
	config_ui_state *state,
	int mod_index,
	const DTTR_Mods_ConfigField *source
) {
	if (state->mod_config_field_count >= DTTR_CONFIG_UI_MOD_CONFIG_FIELDS_MAX
		|| !DTTR_Mods_ConfigField_Valid(source)
		|| has_mod_config_field(state, mod_index, source->id)
		|| !mod_config_field_type_valid(source->type)) {
		return false;
	}

	if (source->type == DTTR_MODS_CONFIG_FIELD_ENUM
		&& (!source->choices || source->choice_count == 0)) {
		return false;
	}

	config_mod_field_ui *field = &state->mod_config_fields[state->mod_config_field_count];

	memset(field, 0, sizeof(*field));

	field->mod_index = mod_index;
	field->type = source->type;
	field->default_value = source->default_value;
	field->int_min = source->int_min;
	field->int_max = source->int_max;
	field->float_min = source->float_min;
	field->float_max = source->float_max;
	field->first_choice = state->mod_config_choice_count;

	if (!copy_ui_string(field->id, sizeof(field->id), source->id, false)
		|| !copy_ui_label_and_tooltip(
			field->label,
			sizeof(field->label),
			source->label,
			source->id,
			field->tooltip,
			sizeof(field->tooltip),
			source->tooltip
		)) {
		return false;
	}

	if (source->type == DTTR_MODS_CONFIG_FIELD_ENUM) {
		if (source->choice_count > DTTR_CONFIG_UI_MOD_CONFIG_CHOICES_MAX
			|| state->mod_config_choice_count + (int)source->choice_count
				   > DTTR_CONFIG_UI_MOD_CONFIG_CHOICES_MAX) {
			return false;
		}

		for (size_t i = 0; i < source->choice_count; i++) {
			if (!append_mod_config_choice(state, &source->choices[i])) {
				return false;
			}
		}

		field->choice_count = (int)source->choice_count;
	}

	if (source->type == DTTR_MODS_CONFIG_FIELD_STRING
		|| source->type == DTTR_MODS_CONFIG_FIELD_ENUM
		|| source->type == DTTR_MODS_CONFIG_FIELD_INPUT_BINDING) {
		if (!copy_ui_string(
				field->default_string,
				sizeof(field->default_string),
				DTTR_ModConfig_StringDefault(source),
				true
			)) {
			return false;
		}

		field->default_value.string_value = field->default_string;
	}

	state->mod_config_field_count++;
	return true;
}

static bool append_mod_config_spec(
	config_ui_state *state,
	const char *dll_name,
	const DTTR_Mods_ConfigSpec *spec
) {
	if (!DTTR_Mods_ConfigSpec_Valid(spec) || has_mod_config(state, spec->mod_id)
		|| state->mod_config_count >= DTTR_CONFIG_MOD_CONFIGS_MAX
		|| state->mod_config_field_count + (int)spec->field_count
			   > DTTR_CONFIG_UI_MOD_CONFIG_FIELDS_MAX) {
		return false;
	}

	const int mod_index = state->mod_config_count;
	const int first_field = state->mod_config_field_count;
	const int choice_count = state->mod_config_choice_count;

	config_mod_ui *mod = &state->mod_configs[mod_index];
	memset(mod, 0, sizeof(*mod));
	mod->schema_version = spec->schema_version;
	mod->first_field = first_field;

	if (!copy_ui_string(mod->dll_name, sizeof(mod->dll_name), dll_name, false)
		|| !copy_ui_string(mod->mod_id, sizeof(mod->mod_id), spec->mod_id, false)
		|| !copy_ui_string(
			mod->label,
			sizeof(mod->label),
			spec->label ? spec->label : spec->mod_id,
			false
		)) {
		return false;
	}

	state->mod_config_count++;

	for (size_t i = 0; i < spec->field_count; i++) {
		if (!append_mod_config_field(state, mod_index, &spec->fields[i])) {
			state->mod_config_count = mod_index;
			state->mod_config_field_count = first_field;
			state->mod_config_choice_count = choice_count;
			return false;
		}
	}

	mod->field_count = state->mod_config_field_count - first_field;
	return true;
}

// Loads a mod DLL with "modules" dir in the search path so mods can resolve their deps.
static HMODULE load_mod_config_library(const config_ui_state *state, const char *dll_path) {
	char dependency_dir[MAX_PATH];
	const bool set_dependency_dir = config_ui_build_mod_dependency_dir(
										state->mods_dir,
										dependency_dir,
										sizeof(dependency_dir)
									)
									&& SetDllDirectoryA(dependency_dir);

	HMODULE module = LoadLibraryExA(dll_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

	if (set_dependency_dir) {
		SetDllDirectoryA(NULL);
	}

	return module;
}

static void scan_mod_config_spec(config_ui_state *state, const char *dll_name) {
#ifdef DTTR_MODS_ENABLED
	if (!state || !state->mods_dir[0] || !dll_name || !dll_name[0]
		|| DTTR_Config_IsModDisabled(&state->config, dll_name)) {
		return;
	}

	char dll_path[MAX_PATH];
	sds dll_path_sds = sdsnew(state->mods_dir);
	if (!dll_path_sds
		|| !DTTR_Path_AppendSegment(&dll_path_sds, dll_name, DTTR_PATH_NATIVE_SEPARATOR)
		|| !DTTR_Path_CopySds(dll_path, sizeof(dll_path), dll_path_sds)) {
		sdsfree(dll_path_sds);
		return;
	}

	sdsfree(dll_path_sds);

	HMODULE module = load_mod_config_library(state, dll_path);
	if (!module) {
		return;
	}

	DTTR_Mods_ConfigFn config_fn = (DTTR_Mods_ConfigFn)
		GetProcAddress(module, "DTTR_Mod_Config");
	if (config_fn) {
		const DTTR_Mods_ConfigSpec *spec = config_fn();
		if (spec && !append_mod_config_spec(state, dll_name, spec)) {
			set_status(state, "Skipped invalid mod config descriptor.");
		}
	}

	FreeLibrary(module);
#endif
}

static bool scan_mod_config_spec_dll(const char *dll_name, void *user_data) {
	scan_mod_config_spec(user_data, dll_name);

	return true;
}

static void apply_mod_field_default(
	DTTR_Config *config,
	const config_mod_ui *mod,
	const config_mod_field_ui *field,
	bool overwrite
) {
	DTTR_ConfigModDefault def;
	if (DTTR_ModConfig_ResolveDefaultValue(
			field->type,
			field->default_value,
			field->default_string,
			&def
		)
			.status
		!= DTTR_OK) {
		return;
	}

	DTTR_Config_ApplyModFieldDefault(config, mod->mod_id, field->id, &def, overwrite);
}

static void apply_mod_config_defaults(
	const config_ui_state *state,
	DTTR_Config *config,
	bool overwrite
) {
	for (int mod_index = 0; mod_index < state->mod_config_count; mod_index++) {
		const config_mod_ui *mod = &state->mod_configs[mod_index];
		DTTR_Config_SetModSchemaVersion(config, mod->mod_id, mod->schema_version);

		for (int i = 0; i < mod->field_count; i++) {
			apply_mod_field_default(
				config,
				mod,
				&state->mod_config_fields[mod->first_field + i],
				overwrite
			);
		}
	}
}

void reload_mod_config_specs(config_ui_state *state) {
	if (!state) {
		return;
	}

	state->mod_config_count = 0;
	state->mod_config_field_count = 0;
	state->mod_config_choice_count = 0;
	for_each_mod_dll(state->mods_dir, scan_mod_config_spec_dll, state);

	DTTR_Config_SetDefaults(&state->defaults);
	apply_mod_config_defaults(state, &state->defaults, true);
	apply_mod_config_defaults(state, &state->config, false);
	apply_mod_config_defaults(state, &state->saved_config, false);
}
