#include "gui_internal.h"

#include <float.h>

static const char *TOOLTIP_HOT_RELOAD = "Hot-reload mod DLLs while the game runs. "
										"Default: false.";
static const char *TOOLTIP_MOD_ENABLE = "Load this mod DLL on the next game launch. "
										"Default: enabled.";
static const char *MODDING_WARNING_TEXT = "The DttR modding API is currently "
										  "experimental and may change "
										  "without warning.";

typedef struct {
	int count;
	char names[DTTR_CONFIG_DISABLED_MODS_MAX][MAX_PATH];
} config_mod_dll_list;

static bool collect_mod_dll(const char *dll_name, void *user_data) {
	config_mod_dll_list *out = user_data;
	if (out->count >= DTTR_CONFIG_DISABLED_MODS_MAX) {
		return false;
	}

	if (DTTR_Path_CopyString(
			out->names[out->count],
			sizeof(out->names[out->count]),
			dll_name
		)) {
		out->count++;
	}

	return true;
}

static void scan_mod_dlls(const config_ui_state *state, config_mod_dll_list *out) {
	SDL_memset(out, 0, sizeof(*out));
	for_each_mod_dll(state ? state->mods_dir : NULL, collect_mod_dll, out);
}

static bool begin_mod_table(void) {
	const ImGuiTableFlags flags = (CONFIG_TABLE_FLAGS & ~ImGuiTableFlags_PadOuterX)
								  | ImGuiTableFlags_NoPadOuterX;
	if (!igBeginTable("##modding_mod_table", 2, flags, (ImVec2_c){0.0f, 0.0f}, 0.0f)) {
		return false;
	}

	igTableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
	igTableSetupColumn("DLL", ImGuiTableColumnFlags_WidthStretch, 0.0f, 0);
	igTableHeadersRow();
	return true;
}

static void draw_mod_toggle_row(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *mod_name,
	int row_index
) {
	begin_config_table_row();
	igTableNextColumn();

	bool enabled = !DTTR_Config_IsModDisabled(&state->config, mod_name);
	igPushID_Int(row_index);
	const bool changed = igCheckbox("##mod_enabled", &enabled);
	igPopID();

	if (changed) {
		DTTR_Result result = DTTR_Config_SetModEnabled(&state->config, mod_name, enabled);
		if (result.status == DTTR_OK) {
			reload_mod_config_specs(state);
		} else {
			set_status(
				state,
				result.message ? result.message : "Could not update mod toggle."
			);
		}
	}

	show_tooltip(TOOLTIP_MOD_ENABLE);

	igTableNextColumn();
	igBeginDisabled(!enabled);
	igTextUnformatted(mod_name, NULL);
	igEndDisabled();
}

static const config_mod_choice_ui *mod_config_choice(
	const config_ui_state *state,
	const config_mod_field_ui *field,
	int index
) {
	if (!state || !field || index < 0 || index >= field->choice_count) {
		return NULL;
	}

	const int choice_index = field->first_choice + index;
	if (choice_index < 0 || choice_index >= state->mod_config_choice_count) {
		return NULL;
	}

	return &state->mod_config_choices[choice_index];
}

static const config_mod_choice_ui *find_mod_config_choice(
	const config_ui_state *state,
	const config_mod_field_ui *field,
	const char *value
) {
	if (!value) {
		return NULL;
	}

	for (int i = 0; i < field->choice_count; i++) {
		const config_mod_choice_ui *choice = mod_config_choice(state, field, i);
		if (choice && SDL_strcmp(choice->value, value) == 0) {
			return choice;
		}
	}

	return NULL;
}

static void clamp_mod_int(const config_mod_field_ui *field, int *value) {
	if (!field || !value || field->int_min >= field->int_max) {
		return;
	}

	if (*value < field->int_min) {
		*value = field->int_min;
	} else if (*value > field->int_max) {
		*value = field->int_max;
	}
}

static void clamp_mod_float(const config_mod_field_ui *field, float *value) {
	if (!field || !value || field->float_min >= field->float_max) {
		return;
	}

	if (*value < field->float_min) {
		*value = field->float_min;
	} else if (*value > field->float_max) {
		*value = field->float_max;
	}
}

static config_label_state mod_field_label_state(
	const config_ui_state *state,
	const config_mod_ui *mod,
	const config_mod_field_ui *field
) {
	return make_config_label_state(
		DTTR_Config_ModFieldChanged(
			&state->config,
			&state->saved_config,
			mod->mod_id,
			field->id
		),
		DTTR_Config_ModFieldChanged(
			&state->config,
			&state->defaults,
			mod->mod_id,
			field->id
		)
	);
}

static bool labeled_enum_combo(
	const config_ui_state *state,
	const config_mod_field_ui *field,
	char *value,
	size_t value_size,
	config_label_state label_state
) {
	const config_mod_choice_ui *selected = find_mod_config_choice(state, field, value);
	const char *preview = selected ? selected->label : value;
	bool changed = false;

	begin_setting_row();
	igAlignTextToFramePadding();
	draw_config_label(field->label, field->tooltip, label_state);
	igTableNextColumn();
	igSetNextItemWidth(-FLT_MIN);

	if (igBeginCombo("##value", preview, ImGuiComboFlags_None)) {
		for (int i = 0; i < field->choice_count; i++) {
			const config_mod_choice_ui *choice = mod_config_choice(state, field, i);
			if (!choice) {
				continue;
			}

			const bool is_selected = selected
									 && SDL_strcmp(choice->value, selected->value) == 0;
			if (igSelectable_Bool(
					choice->label,
					is_selected,
					ImGuiSelectableFlags_None,
					(ImVec2_c){0.0f, 0.0f}
				)) {
				snprintf(value, value_size, "%s", choice->value);
				selected = choice;
				changed = true;
			}

			show_tooltip(choice->tooltip);

			if (is_selected) {
				igSetItemDefaultFocus();
			}
		}

		igEndCombo();
	}

	show_tooltip(field->tooltip);
	return changed;
}

static void get_mod_string_value(
	const config_ui_state *state,
	const config_mod_ui *mod,
	const config_mod_field_ui *field,
	char *out,
	size_t out_size
) {
	if (DTTR_Config_GetModString(&state->config, mod->mod_id, field->id, out, out_size)
			.status
		!= DTTR_OK) {
		snprintf(out, out_size, "%s", field->default_string);
	}
}

static void draw_mod_input_binding_field(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const config_mod_ui *mod,
	const config_mod_field_ui *field,
	config_label_state label_state
) {
	char token[DTTR_CONFIG_MOD_STRING_MAX] = {0};
	get_mod_string_value(state, mod, field, token, sizeof(token));

	DTTR_Mods_ConfigInputBinding binding = {0};
	DTTR_InputBinding_Parse(token, &binding);

	char display[64];
	if (DTTR_InputBinding_DisplayName(&binding, display, sizeof(display)).status
		!= DTTR_OK) {
		snprintf(display, sizeof(display), "Unbound");
	}

	const bool capturing = input_binding_field_capturing(state, mod->mod_id, field->id);

	const config_binding_row_result row = draw_config_binding_row(
		ctx,
		&(config_binding_row_spec){
			.label = field->label,
			.tooltip = field->tooltip,
			.label_state = label_state,
			.display = display,
			.capture_display = "Press any input...",
			.capturing = capturing,
			.show_clear_button = true,
			.show_reset_button = true,
			.bind_tooltip = "Click, then press a key, mouse button, or gamepad button.",
			.clear_tooltip = "Leave this binding unbound.",
			.reset_tooltip = "Reset to the mod's default binding.",
		}
	);

	if (row.bind_clicked) {
		begin_input_binding_capture(state, mod->mod_id, field->id);
	}

	if (row.clear_clicked) {
		DTTR_Config_SetModString(&state->config, mod->mod_id, field->id, "");
	}

	if (row.reset_clicked) {
		DTTR_Config_SetModString(
			&state->config,
			mod->mod_id,
			field->id,
			field->default_string
		);
	}
}

static void draw_mod_config_field(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const config_mod_ui *mod,
	const config_mod_field_ui *field
) {
	const config_label_state label_state = mod_field_label_state(state, mod, field);

	igPushID_Str(field->id);

	switch (field->type) {
	case DTTR_MODS_CONFIG_FIELD_BOOL: {
		bool value = field->default_value.bool_value;
		DTTR_Config_GetModBool(&state->config, mod->mod_id, field->id, &value);
		if (labeled_checkbox(
				ctx,
				field->label,
				"##value",
				&value,
				field->tooltip,
				label_state
			)) {
			DTTR_Config_SetModBool(&state->config, mod->mod_id, field->id, value);
		}

		break;
	}
	case DTTR_MODS_CONFIG_FIELD_INT: {
		int value = field->default_value.int_value;
		DTTR_Config_GetModInt(&state->config, mod->mod_id, field->id, &value);
		if (labeled_input_int(
				ctx,
				field->label,
				"##value",
				&value,
				1,
				10,
				field->tooltip,
				label_state
			)) {
			clamp_mod_int(field, &value);
			DTTR_Config_SetModInt(&state->config, mod->mod_id, field->id, value);
		}

		break;
	}
	case DTTR_MODS_CONFIG_FIELD_FLOAT: {
		float value = field->default_value.float_value;
		DTTR_Config_GetModFloat(&state->config, mod->mod_id, field->id, &value);
		if (labeled_input_float(
				ctx,
				field->label,
				"##value",
				&value,
				field->tooltip,
				label_state
			)) {
			clamp_mod_float(field, &value);
			DTTR_Config_SetModFloat(&state->config, mod->mod_id, field->id, value);
		}

		break;
	}
	case DTTR_MODS_CONFIG_FIELD_STRING: {
		char value[DTTR_CONFIG_MOD_STRING_MAX] = {0};
		get_mod_string_value(state, mod, field, value, sizeof(value));
		if (labeled_input_text(
				ctx,
				field->label,
				"##value",
				value,
				sizeof(value),
				field->tooltip,
				label_state
			)) {
			DTTR_Result result = DTTR_Config_SetModString(
				&state->config,
				mod->mod_id,
				field->id,
				value
			);
			if (result.status != DTTR_OK) {
				set_status(
					state,
					result.message ? result.message : "Could not update mod config text."
				);
			}
		}

		break;
	}
	case DTTR_MODS_CONFIG_FIELD_ENUM: {
		char value[DTTR_CONFIG_MOD_STRING_MAX] = {0};
		get_mod_string_value(state, mod, field, value, sizeof(value));
		if (!find_mod_config_choice(state, field, value) && field->choice_count > 0) {
			const config_mod_choice_ui *choice = mod_config_choice(state, field, 0);
			if (choice) {
				snprintf(value, sizeof(value), "%s", choice->value);
				DTTR_Config_SetModString(&state->config, mod->mod_id, field->id, value);
			}
		}

		if (labeled_enum_combo(state, field, value, sizeof(value), label_state)) {
			DTTR_Config_SetModString(&state->config, mod->mod_id, field->id, value);
		}

		break;
	}
	case DTTR_MODS_CONFIG_FIELD_INPUT_BINDING:
		draw_mod_input_binding_field(ctx, state, mod, field, label_state);
		break;
	default:
		break;
	}

	igPopID();
}

static void draw_mod_config_tab(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	int mod_index
) {
	const config_mod_ui *mod = &state->mod_configs[mod_index];
	char tab_label
		[DTTR_CONFIG_UI_MOD_CONFIG_LABEL_MAX + DTTR_CONFIG_MOD_ID_MAX + 4]; // "###" + NUL

	snprintf(tab_label, sizeof(tab_label), "%s###%s", mod->label, mod->mod_id);
	const bool tab_open = igBeginTabItem(tab_label, NULL, ImGuiTabItemFlags_None);
	show_tooltip(mod->mod_id);

	if (!tab_open) {
		return;
	}

	igPushID_Str(mod->mod_id);

	if (mod->field_count == 0) {
		igTextDisabled("No configurable fields.");
	} else if (begin_tab_settings_table(ctx, "##mod_config_table", DTTR_CONFIG_UI_INPUT_W)) {
		for (int i = 0; i < mod->field_count; i++) {
			draw_mod_config_field(
				ctx,
				state,
				mod,
				&state->mod_config_fields[mod->first_field + i]
			);
		}

		end_settings_table();
	}

	igPopID();
	igEndTabItem();
}

static void draw_mod_config_sections(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state
) {
	igSpacing();
	igSeparatorText("Mod Configs");

	if (state->mod_config_count == 0) {
		igTextWrapped("No mod config exports found.");
		return;
	}

	if (!igBeginTabBar(
			"##mod_config_tabs",
			ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip
		)) {
		return;
	}

	for (int i = 0; i < state->mod_config_count; i++) {
		draw_mod_config_tab(ctx, state, i);
	}

	igEndTabBar();
}

static void draw_mod_section_header(void) {
	igSpacing();
	igSeparatorText("Mods");
	show_tooltip(TOOLTIP_MOD_ENABLE);
}

void draw_modding_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_CONFIG_UI_WARNING_TEXT_COLOR);
	igTextWrapped("%s", MODDING_WARNING_TEXT);
	igPopStyleColor(1);
	igSpacing();

	if (!begin_tab_settings_table(
			ctx,
			"##modding_settings_table",
			DTTR_CONFIG_UI_INPUT_W
		)) {
		return;
	}

	labeled_checkbox(
		ctx,
		"Hot Reload",
		"##hot_reload",
		&state->config.hot_reload,
		TOOLTIP_HOT_RELOAD,
		FIELD_LABEL_STATE(state, hot_reload)
	);
	end_settings_table();

	config_mod_dll_list mods;
	scan_mod_dlls(state, &mods);

	draw_mod_section_header();

	if (mods.count == 0) {
		igTextWrapped(
			"No mod DLLs found in %s",
			state->mods_dir[0] ? state->mods_dir : "mods"
		);
		draw_mod_config_sections(ctx, state);
		return;
	}

	if (!begin_mod_table()) {
		return;
	}

	for (int i = 0; i < mods.count; i++) {
		draw_mod_toggle_row(ctx, state, mods.names[i], i);
	}

	igEndTable();
	draw_mod_config_sections(ctx, state);
}
