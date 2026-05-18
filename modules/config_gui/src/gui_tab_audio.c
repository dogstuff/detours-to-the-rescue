#include "gui_internal.h"

static const char *TOOLTIP_MSS_SAMPLE_GAIN = "MSS sample gain. Default: 1.0.";
static const char *TOOLTIP_MSS_SAMPLE_PREEMPHASIS = "MSS sample preemphasis. Default: "
													"0.0.";

void draw_audio_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!begin_tab_settings_table(ctx, "##audio_settings_table", DTTR_CONFIG_UI_INPUT_W)) {
		return;
	}

	labeled_input_float(
		ctx,
		"MSS sample gain",
		"##mss_sample_gain",
		&state->config.mss_sample_gain,
		TOOLTIP_MSS_SAMPLE_GAIN,
		FIELD_LABEL_STATE(state, mss_sample_gain)
	);
	labeled_input_float(
		ctx,
		"MSS sample preemphasis",
		"##mss_sample_preemphasis",
		&state->config.mss_sample_preemphasis,
		TOOLTIP_MSS_SAMPLE_PREEMPHASIS,
		FIELD_LABEL_STATE(state, mss_sample_preemphasis)
	);
	end_settings_table();
}
