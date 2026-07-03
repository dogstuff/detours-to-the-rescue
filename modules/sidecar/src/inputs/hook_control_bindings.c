#include <SDL3/SDL.h>
#include <dttr_config.h>
#include <dttr_log.h>
#include <dttr_pcdogs.h>

#include "hooks_private.h"
#include "inputs_private.h"

DTTR_PCDOGS_F_Input_RegisterButtonMapping_proto
	dttr_inputs_hook_register_button_mapping_original;
DTTR_PCDOGS_F_Config_ApplySettings_proto dttr_inputs_hook_config_apply_settings_original;
DTTR_PCDOGS_F_Input_ReadDevices_proto dttr_inputs_hook_read_devices_original;

static uint32_t custom_sdl_button_masks[DTTR_INPUTS_SDL_BUTTON_COUNT];

bool dttr_inputs_hook_mapping_prepare(const DTTR_Mods_Context *ctx) {
	return ctx && DTTR_PCDOGS_F_Input_RegisterButtonMapping->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_F_Config_ApplySettings->IsCallable(&ctx->runtime);
}

bool dttr_inputs_hook_read_devices_prepare(const DTTR_Mods_Context *ctx) {
	return ctx && DTTR_PCDOGS_F_Input_ReadDevices->IsCallable(&ctx->runtime);
}

void dttr_inputs_custom_button_mappings_clear() {
	SDL_zeroa(custom_sdl_button_masks);
}

uint32_t dttr_inputs_custom_button_mapping_mask(int button) {
	if (button < 0 || button >= DTTR_INPUTS_SDL_BUTTON_COUNT) {
		return 0;
	}

	return custom_sdl_button_masks[button];
}

void dttr_inputs_hook_mapping_reset() {
	dttr_inputs_hook_register_button_mapping_original = NULL;
	dttr_inputs_hook_config_apply_settings_original = NULL;
	dttr_inputs_hook_read_devices_original = NULL;
	dttr_inputs_custom_button_mappings_clear();
}

void dttr_inputs_apply_custom_button_mappings(DTTR_PCDOGS_T_Input_State *state) {
	if (!state) {
		return;
	}

	for (int button = 0; button < DTTR_INPUTS_SDL_BUTTON_COUNT; button++) {
		const uint32_t mask = custom_sdl_button_masks[button];
		if (mask && dttr_inputs_controller_button_pressed(button)) {
			state->button_bits |= mask;
		}
	}
}

int32_t __cdecl dttr_inputs_hook_register_button_mapping_callback(
	int32_t control_code,
	uint32_t button_mask
) {
	const DTTR_Input_KeyCodeKind kind = dttr_inputs_key_code_kind(control_code);
	if (kind == DTTR_INPUTS_KEY_CODE_SDL_GAMEPAD) {
		const int button = dttr_inputs_key_code_sdl_button(control_code);
		custom_sdl_button_masks[button] |= button_mask;
		return 0;
	}

	if (kind == DTTR_INPUTS_KEY_CODE_NONE
		&& control_code >= DTTR_INPUTS_GAMEPAD_BUTTON_BASE) {
		DTTR_LOG_WARN(
			"Dropped out-of-range gamepad control code %d for mask 0x%X",
			control_code,
			button_mask
		);
		return 0;
	}

	return dttr_inputs_hook_register_button_mapping_original
			   ? dttr_inputs_hook_register_button_mapping_original(
					 control_code,
					 button_mask
				 )
			   : 0;
}

static void register_start_pause_override() {
	const int action = DTTR_Config_ControlActionIndex("start_pause");
	if (action < 0) {
		return;
	}

	const int code = dttr_config.control_bindings[action];
	if (code == DTTR_CONFIG_CONTROL_BINDING_NONE) {
		return;
	}

	dttr_inputs_hook_register_button_mapping_callback(
		code,
		DTTR_Config_ControlActionButtonMask(action)
	);
}

void __cdecl dttr_inputs_hook_config_apply_settings_callback() {
	dttr_inputs_custom_button_mappings_clear();

	if (dttr_inputs_hook_config_apply_settings_original) {
		dttr_inputs_hook_config_apply_settings_original();
	}

	register_start_pause_override();
}

void __cdecl dttr_inputs_hook_read_devices_callback(
	int32_t player_index,
	DTTR_PCDOGS_T_Input_State *state
) {
	if (dttr_inputs_hook_read_devices_original) {
		dttr_inputs_hook_read_devices_original(player_index, state);
	}

	dttr_inputs_apply_custom_button_mappings(state);
}
