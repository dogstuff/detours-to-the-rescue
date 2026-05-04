#include "gui_internal.h"

static const char *S_TOOLTIP_MSS_SDL_ENABLED = "Use the SDL3_mixer MSS32 shim. Default: "
											   "true.";
static const char *S_TOOLTIP_MSS_SAMPLE_GAIN = "MSS sample gain and preemphasis. "
											   "Default: 1.0.";
static const char *S_TOOLTIP_MSS_SAMPLE_PREEMPHASIS = "MSS sample gain and preemphasis. "
													  "Default: 0.0.";

void s_draw_audio_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	if (!s_begin_tab_settings_table(
			ctx,
			"##audio_settings_table",
			DTTR_CONFIG_UI_INPUT_W
		)) {
		return;
	}

	s_labeled_checkbox(
		ctx,
		"Use SDL3_mixer MSS shim",
		"##mss_sdl_enabled",
		&state->m_config.m_mss_sdl_enabled,
		S_TOOLTIP_MSS_SDL_ENABLED,
		S_FIELD_LABEL_STATE(state, m_mss_sdl_enabled)
	);
	s_labeled_input_float(
		ctx,
		"MSS sample gain",
		"##mss_sample_gain",
		&state->m_config.m_mss_sample_gain,
		S_TOOLTIP_MSS_SAMPLE_GAIN,
		S_FIELD_LABEL_STATE(state, m_mss_sample_gain)
	);
	s_labeled_input_float(
		ctx,
		"MSS sample preemphasis",
		"##mss_sample_preemphasis",
		&state->m_config.m_mss_sample_preemphasis,
		S_TOOLTIP_MSS_SAMPLE_PREEMPHASIS,
		S_FIELD_LABEL_STATE(state, m_mss_sample_preemphasis)
	);
	s_end_settings_table();
}
