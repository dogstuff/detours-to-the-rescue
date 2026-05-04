#include "gui_internal.h"

static const char *const S_GRAPHICS_API_TOOLTIPS[] = {
	"Uses the best available graphics backend.",
	"Uses the Vulkan graphics backend.",
	"Uses the Direct3D 12 graphics backend.",
	"Uses the OpenGL graphics backend.",
};

static const char *const S_SCALING_FIT_TOOLTIPS[] = {
	"Preserves aspect ratio and adds bars as needed.",
	"Fills the whole window even if the image is distorted.",
	"Uses whole-number scaling while preserving aspect ratio.",
};

static const char *const S_SCALING_METHOD_TOOLTIPS[] = {
	"Preserves the game's native resolution and scales finished frames.",
	"Scales inputs in render calls to render at a higher resolution.",
};

static const char *const S_PRESENT_FILTER_TOOLTIPS[] = {
	"Uses nearest-neighbor sampling when presenting frames.",
	"Uses linear filtering when presenting frames.",
};

static const char *const S_VERTEX_PRECISION_TOOLTIPS[] = {
	"Uses the game's original vertex coordinate precision.",
	"Uses subpixel vertex coordinates for smoother 3D movement.\n\nNOTE: Subpixel mode "
	"may make certain seams between polygons on the puppy models visible at higher "
	"resolutions.",
};

static const char *S_TOOLTIP_GRAPHICS_API = "The GPU backend to use. Default: auto.";
static const char *S_TOOLTIP_WINDOW_WIDTH = "The initial width of the game window in "
											"pixels. Default: 640.";
static const char *S_TOOLTIP_WINDOW_HEIGHT = "The initial height of the game window in "
											 "pixels. Default: 480.";
static const char *S_TOOLTIP_SCALING_FIT = "How the game should scale to fit the window. "
										   "Default: letterbox.";
static const char *S_TOOLTIP_SCALING_METHOD = "The type of scaling to use when the "
											  "window size exceeds the game resolution. "
											  "Default: logical.";
static const char *S_TOOLTIP_PRESENT_FILTER = "The filtering algorithm used when scaling "
											  "the rendered frame to the display. "
											  "Default: linear.";
static const char *S_TOOLTIP_VERTEX_PRECISION = "How vertex coordinates are computed "
												"during 3D rendering. Default: native.";
static const char *S_TOOLTIP_SPRITE_SMOOTH = "Use smoothing for 2D sprites if true, else "
											 "use nearest-neighbor. Default: true.";
static const char *S_TOOLTIP_MSAA_SAMPLES = "Number of multisample anti-aliasing "
											"samples. Set to 1 to disable. Default: 2.";
static const char *S_TOOLTIP_TEXTURE_UPLOAD_SYNC = "Wait for each texture upload to "
												   "finish before continuing. Default: "
												   "false.";
static const char *S_TOOLTIP_GENERATE_TEXTURE_MIPMAPS
	= "Generate mipmaps for textures to reduce aliasing at smaller sizes. Default: true.";
static const char *S_TOOLTIP_FULLSCREEN = "Whether to start the game in fullscreen mode. "
										  "Press F11 to toggle at runtime. Default: "
										  "false.";

void s_draw_graphics_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	if (!s_begin_tab_settings_table(
			ctx,
			"##graphics_settings_table",
			DTTR_CONFIG_UI_INPUT_W
		)) {
		return;
	}

	s_labeled_choice_combo(
		ctx,
		"Graphics API",
		"##graphics_api",
		(int *)&state->m_config.m_graphics_api,
		DTTR_CONFIG_CHOICES_GRAPHICS_API,
		S_GRAPHICS_API_TOOLTIPS,
		S_TOOLTIP_GRAPHICS_API,
		S_FIELD_LABEL_STATE(state, m_graphics_api)
	);
	s_labeled_input_int(
		ctx,
		"Window width",
		"##window_width",
		&state->m_config.m_window_width,
		1,
		100,
		S_TOOLTIP_WINDOW_WIDTH,
		S_FIELD_LABEL_STATE(state, m_window_width)
	);
	s_labeled_input_int(
		ctx,
		"Window height",
		"##window_height",
		&state->m_config.m_window_height,
		1,
		100,
		S_TOOLTIP_WINDOW_HEIGHT,
		S_FIELD_LABEL_STATE(state, m_window_height)
	);
	s_labeled_choice_combo(
		ctx,
		"Scaling fit",
		"##scaling_fit",
		(int *)&state->m_config.m_scaling_fit,
		DTTR_CONFIG_CHOICES_SCALING_FIT,
		S_SCALING_FIT_TOOLTIPS,
		S_TOOLTIP_SCALING_FIT,
		S_FIELD_LABEL_STATE(state, m_scaling_fit)
	);
	s_labeled_choice_combo(
		ctx,
		"Scaling method",
		"##scaling_method",
		(int *)&state->m_config.m_scaling_method,
		DTTR_CONFIG_CHOICES_SCALING_METHOD,
		S_SCALING_METHOD_TOOLTIPS,
		S_TOOLTIP_SCALING_METHOD,
		S_FIELD_LABEL_STATE(state, m_scaling_method)
	);
	s_labeled_choice_combo(
		ctx,
		"Present scaling algorithm",
		"##present_filter",
		(int *)&state->m_config.m_present_filter,
		DTTR_CONFIG_CHOICES_PRESENT_FILTER,
		S_PRESENT_FILTER_TOOLTIPS,
		S_TOOLTIP_PRESENT_FILTER,
		S_FIELD_LABEL_STATE(state, m_present_filter)
	);
	s_labeled_choice_combo(
		ctx,
		"Vertex precision",
		"##vertex_precision",
		(int *)&state->m_config.m_vertex_precision,
		DTTR_CONFIG_CHOICES_VERTEX_PRECISION,
		S_VERTEX_PRECISION_TOOLTIPS,
		S_TOOLTIP_VERTEX_PRECISION,
		S_FIELD_LABEL_STATE(state, m_vertex_precision)
	);
	s_labeled_checkbox(
		ctx,
		"Smooth 2D sprites",
		"##sprite_smooth",
		&state->m_config.m_sprite_smooth,
		S_TOOLTIP_SPRITE_SMOOTH,
		S_FIELD_LABEL_STATE(state, m_sprite_smooth)
	);
	s_labeled_input_int(
		ctx,
		"MSAA samples",
		"##msaa_samples",
		&state->m_config.m_msaa_samples,
		1,
		4,
		S_TOOLTIP_MSAA_SAMPLES,
		S_FIELD_LABEL_STATE(state, m_msaa_samples)
	);
	s_labeled_checkbox(
		ctx,
		"Texture upload sync",
		"##texture_upload_sync",
		&state->m_config.m_texture_upload_sync,
		S_TOOLTIP_TEXTURE_UPLOAD_SYNC,
		S_FIELD_LABEL_STATE(state, m_texture_upload_sync)
	);
	s_labeled_checkbox(
		ctx,
		"Generate texture mipmaps",
		"##generate_texture_mipmaps",
		&state->m_config.m_generate_texture_mipmaps,
		S_TOOLTIP_GENERATE_TEXTURE_MIPMAPS,
		S_FIELD_LABEL_STATE(state, m_generate_texture_mipmaps)
	);
	s_labeled_checkbox(
		ctx,
		"Fullscreen",
		"##fullscreen",
		&state->m_config.m_fullscreen,
		S_TOOLTIP_FULLSCREEN,
		S_FIELD_LABEL_STATE(state, m_fullscreen)
	);
	s_end_settings_table();
}
