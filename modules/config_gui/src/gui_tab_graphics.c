#include "gui_internal.h"

static const char *const GRAPHICS_API_TOOLTIPS[] = {
	"Uses the best available graphics backend.",
	"Uses the Vulkan graphics backend.",
	"Uses the Direct3D 12 graphics backend.",
	"Uses the OpenGL graphics backend.",
};

static const char *const SCALING_FIT_TOOLTIPS[] = {
	"Preserves aspect ratio and adds bars as needed.",
	"Fills the whole window even if the image is distorted.",
	"Uses whole-number scaling while preserving aspect ratio.",
};

static const char *const SCALING_METHOD_TOOLTIPS[] = {
	"Preserves the game's native resolution and scales finished frames.",
	"Scales inputs in render calls to render at a higher resolution.",
};

static const char *const PRESENT_FILTER_TOOLTIPS[] = {
	"Uses nearest-neighbor sampling when presenting frames.",
	"Uses linear filtering when presenting frames.",
};

static const char *const VERTEX_PRECISION_TOOLTIPS[] = {
	"Uses the game's original vertex coordinate precision.",
	"Uses subpixel vertex coordinates for smoother 3D movement.\n\nNOTE: Subpixel mode "
	"may make certain seams between polygons on the puppy models visible at higher "
	"resolutions.",
};

static const char *TOOLTIP_GRAPHICS_API = "The GPU backend to use. Default: auto.";
static const char *TOOLTIP_WINDOW_WIDTH = "The initial width of the game window in "
										  "pixels. Default: 640.";
static const char *TOOLTIP_WINDOW_HEIGHT = "The initial height of the game window in "
										   "pixels. Default: 480.";
static const char *TOOLTIP_SCALING_FIT = "How the game should scale to fit the window. "
										 "Default: letterbox.";
static const char *TOOLTIP_SCALING_METHOD = "The type of scaling to use when the "
											"window size exceeds the game resolution. "
											"Default: logical.";
static const char *TOOLTIP_PRESENT_FILTER = "The filtering algorithm used when scaling "
											"the rendered frame to the display. "
											"Default: linear.";
static const char *TOOLTIP_VERTEX_PRECISION = "How vertex coordinates are computed "
											  "during 3D rendering. Default: native.";
static const char *TOOLTIP_SPRITE_SMOOTH = "Use smoothing for 2D sprites if true, else "
										   "use nearest-neighbor. Default: true.";
static const char *TOOLTIP_MSAA_SAMPLES = "Number of multisample anti-aliasing "
										  "samples. Set to 1 to disable. Default: 2.";
static const char *TOOLTIP_TEXTURE_UPLOAD_SYNC = "Wait for each texture upload to "
												 "finish before continuing. Default: "
												 "false.";
static const char *TOOLTIP_GENERATE_TEXTURE_MIPMAPS
	= "Generate mipmaps for textures to reduce aliasing at smaller sizes. Default: true.";
static const char *TOOLTIP_FULLSCREEN = "Whether to start the game in fullscreen mode. "
										"Press F11 to toggle at runtime. Default: "
										"false.";

void draw_graphics_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!begin_tab_settings_table(
			ctx,
			"##graphics_settings_table",
			DTTR_CONFIG_UI_INPUT_W
		)) {
		return;
	}

	labeled_choice_combo(
		ctx,
		"Graphics API",
		"##graphics_api",
		(int *)&state->config.graphics_api,
		DTTR_CONFIG_CHOICES_GRAPHICS_API,
		GRAPHICS_API_TOOLTIPS,
		TOOLTIP_GRAPHICS_API,
		FIELD_LABEL_STATE(state, graphics_api)
	);
	labeled_input_int(
		ctx,
		"Window width",
		"##window_width",
		&state->config.window_width,
		1,
		100,
		TOOLTIP_WINDOW_WIDTH,
		FIELD_LABEL_STATE(state, window_width)
	);
	labeled_input_int(
		ctx,
		"Window height",
		"##window_height",
		&state->config.window_height,
		1,
		100,
		TOOLTIP_WINDOW_HEIGHT,
		FIELD_LABEL_STATE(state, window_height)
	);
	labeled_choice_combo(
		ctx,
		"Scaling fit",
		"##scaling_fit",
		(int *)&state->config.scaling_fit,
		DTTR_CONFIG_CHOICES_SCALING_FIT,
		SCALING_FIT_TOOLTIPS,
		TOOLTIP_SCALING_FIT,
		FIELD_LABEL_STATE(state, scaling_fit)
	);
	labeled_choice_combo(
		ctx,
		"Scaling method",
		"##scaling_method",
		(int *)&state->config.scaling_method,
		DTTR_CONFIG_CHOICES_SCALING_METHOD,
		SCALING_METHOD_TOOLTIPS,
		TOOLTIP_SCALING_METHOD,
		FIELD_LABEL_STATE(state, scaling_method)
	);
	labeled_choice_combo(
		ctx,
		"Present scaling algorithm",
		"##present_filter",
		(int *)&state->config.present_filter,
		DTTR_CONFIG_CHOICES_PRESENT_FILTER,
		PRESENT_FILTER_TOOLTIPS,
		TOOLTIP_PRESENT_FILTER,
		FIELD_LABEL_STATE(state, present_filter)
	);
	labeled_choice_combo(
		ctx,
		"Vertex precision",
		"##vertex_precision",
		(int *)&state->config.vertex_precision,
		DTTR_CONFIG_CHOICES_VERTEX_PRECISION,
		VERTEX_PRECISION_TOOLTIPS,
		TOOLTIP_VERTEX_PRECISION,
		FIELD_LABEL_STATE(state, vertex_precision)
	);
	labeled_checkbox(
		ctx,
		"Smooth 2D sprites",
		"##sprite_smooth",
		&state->config.sprite_smooth,
		TOOLTIP_SPRITE_SMOOTH,
		FIELD_LABEL_STATE(state, sprite_smooth)
	);
	labeled_input_int(
		ctx,
		"MSAA samples",
		"##msaa_samples",
		&state->config.msaa_samples,
		1,
		4,
		TOOLTIP_MSAA_SAMPLES,
		FIELD_LABEL_STATE(state, msaa_samples)
	);
	labeled_checkbox(
		ctx,
		"Texture upload sync",
		"##texture_upload_sync",
		&state->config.texture_upload_sync,
		TOOLTIP_TEXTURE_UPLOAD_SYNC,
		FIELD_LABEL_STATE(state, texture_upload_sync)
	);
	labeled_checkbox(
		ctx,
		"Generate texture mipmaps",
		"##generate_texture_mipmaps",
		&state->config.generate_texture_mipmaps,
		TOOLTIP_GENERATE_TEXTURE_MIPMAPS,
		FIELD_LABEL_STATE(state, generate_texture_mipmaps)
	);
	labeled_checkbox(
		ctx,
		"Fullscreen",
		"##fullscreen",
		&state->config.fullscreen,
		TOOLTIP_FULLSCREEN,
		FIELD_LABEL_STATE(state, fullscreen)
	);
	end_settings_table();
}
