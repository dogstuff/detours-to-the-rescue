#include "gui_internal.h"

static const char *TOOLTIP_SAMPLE_GAIN = "Sample Gain. Default: 1.0.";
static const char *TOOLTIP_DIRECTSOUND_DELAY = "Add DirectSound-style sample latency to "
											   "the audio shim. Default: off.";

void draw_audio_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!begin_tab_settings_table(ctx, "##audio_settings_table", DTTR_CONFIG_UI_INPUT_W)) {
		return;
	}

	labeled_input_float(
		ctx,
		"Sample Gain",
		"##mss_sample_gain",
		&state->config.mss_sample_gain,
		TOOLTIP_SAMPLE_GAIN,
		FIELD_LABEL_STATE(state, mss_sample_gain)
	);
	labeled_checkbox(
		ctx,
		"Simulate DirectSound Delay",
		"##mss_simulate_directsound_delay",
		&state->config.mss_simulate_directsound_delay,
		TOOLTIP_DIRECTSOUND_DELAY,
		FIELD_LABEL_STATE(state, mss_simulate_directsound_delay)
	);
	end_settings_table();
}
