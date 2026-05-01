#include <dttr_imgui.h>
#include <dttr_loader_ui.h>
#include <dttr_sdl.h>

#include <stdbool.h>
#include <stdio.h>

#define DTTR_LOADER_UI_WINDOW_W 560
#define DTTR_LOADER_UI_WINDOW_H 190
#define DTTR_LOADER_UI_BUTTON_W 160.0f
#define DTTR_LOADER_UI_BUTTON_H 28.0f
#define DTTR_LOADER_UI_BUTTON_SPACING 8.0f
#define DTTR_LOADER_UI_TEXT_PADDING_X 18.0f
#define DTTR_LOADER_UI_TEXT_PADDING_Y 16.0f

#ifndef DTTR_VERSION
#define DTTR_VERSION "unknown"
#endif

static const char *const S_WINDOW_TITLE = "DttR: Specify Game Files";
static const char *const S_HEADER_TITLE = "102 Patches: Detours to the Rescue!";
static const char *const S_GAME_SOURCE_MESSAGE
	= "Select either a directory containing the 102 Dalmatians: Puppies to the Rescue "
	  "files, the original game disc, or an ISO image.";
static const char *const S_ERROR_TITLE = "DttR: Error";

static DTTR_LoaderUIChoice s_native_choose_game_source(void) {
	const SDL_MessageBoxButtonData buttons[] = {
		{0, DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER, "Open Directory"},
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		 DTTR_LOADER_UI_CHOICE_BROWSE_ISO,
		 "Open ISO"},
	};

	const SDL_MessageBoxData msgbox = {
		.flags = SDL_MESSAGEBOX_INFORMATION,
		.window = NULL,
		.title = S_WINDOW_TITLE,
		.message = S_GAME_SOURCE_MESSAGE,
		.numbuttons = 2,
		.buttons = buttons,
	};

	int choice_id = DTTR_LOADER_UI_CHOICE_EXIT;
	if (!dttr_sdl_show_message_box(&msgbox, &choice_id)) {
		return DTTR_LOADER_UI_CHOICE_EXIT;
	}

	return dttr_loader_ui_choice_from_id(choice_id);
}

static float s_button_row_width(const DTTR_ImGuiDialogContext *ctx, size_t button_count) {
	if (button_count == 0) {
		return 0.0f;
	}

	return dttr_imgui_dialog_scaled_float(ctx, DTTR_LOADER_UI_BUTTON_W)
			   * (float)button_count
		   + dttr_imgui_dialog_scaled_float(ctx, DTTR_LOADER_UI_BUTTON_SPACING)
				 * (float)(button_count - 1);
}

static ImVec2_c s_button_size(const DTTR_ImGuiDialogContext *ctx) {
	return (ImVec2_c){
		dttr_imgui_dialog_scaled_float(ctx, DTTR_LOADER_UI_BUTTON_W),
		dttr_imgui_dialog_scaled_float(ctx, DTTR_LOADER_UI_BUTTON_H),
	};
}

static bool s_draw_source_button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	DTTR_LoaderUIChoice choice,
	DTTR_LoaderUIChoice *result,
	bool *running
) {
	if (!dttr_imgui_dialog_button(ctx, id, label, s_button_size(ctx))) {
		return false;
	}

	*result = choice;
	*running = false;
	return true;
}

static bool s_draw_disc_button(
	const DTTR_ImGuiDialogContext *ctx,
	size_t disc_index,
	const DTTR_LoaderUIDiscCandidate *disc_candidates,
	DTTR_LoaderUIChoice *result,
	bool *running
) {
	char button_id[32];
	snprintf(button_id, sizeof(button_id), "##disc_%u", (unsigned)disc_index);
	return s_draw_source_button(
		ctx,
		button_id,
		disc_candidates[disc_index].m_label,
		dttr_loader_ui_disc_choice_for_index(disc_index),
		result,
		running
	);
}

static void s_same_button_row(const DTTR_ImGuiDialogContext *ctx) {
	igSameLine(0.0f, dttr_imgui_dialog_scaled_float(ctx, DTTR_LOADER_UI_BUTTON_SPACING));
}

static size_t s_clamp_disc_candidate_count(size_t disc_candidate_count) {
	if (disc_candidate_count > DTTR_LOADER_UI_MAX_DISC_CANDIDATES) {
		return DTTR_LOADER_UI_MAX_DISC_CANDIDATES;
	}

	return disc_candidate_count;
}

static void s_draw_browse_buttons(
	const DTTR_ImGuiDialogContext *ctx,
	DTTR_LoaderUIChoice *result,
	bool *running
) {
	if (!*running) {
		return;
	}

	s_draw_source_button(
		ctx,
		"##open_directory",
		"Open Directory",
		DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER,
		result,
		running
	);
	if (!*running) {
		return;
	}

	s_same_button_row(ctx);
	s_draw_source_button(
		ctx,
		"##open_iso",
		"Open ISO",
		DTTR_LOADER_UI_CHOICE_BROWSE_ISO,
		result,
		running
	);
}

static void s_draw_source_buttons(
	const DTTR_ImGuiDialogContext *ctx,
	const DTTR_LoaderUIDiscCandidate *disc_candidates,
	size_t disc_candidate_count,
	DTTR_LoaderUIChoice *result,
	bool *running
) {
	size_t disc_index = 0;
	const size_t first_row_buttons = disc_candidate_count > 0 ? 3 : 2;
	dttr_imgui_dialog_center_next_item(s_button_row_width(ctx, first_row_buttons));

	if (disc_candidate_count > 0) {
		s_draw_disc_button(ctx, disc_index++, disc_candidates, result, running);
		s_same_button_row(ctx);
	}

	s_draw_browse_buttons(ctx, result, running);

	while (disc_index < disc_candidate_count && *running) {
		const size_t remaining = disc_candidate_count - disc_index;
		const size_t row_count = remaining < 3 ? remaining : 3;
		dttr_imgui_dialog_center_next_item(s_button_row_width(ctx, row_count));

		for (size_t col = 0; col < row_count && *running; col++, disc_index++) {
			if (col > 0) {
				s_same_button_row(ctx);
			}

			s_draw_disc_button(ctx, disc_index, disc_candidates, result, running);
		}
	}
}

DTTR_LoaderUIChoice dttr_loader_ui_choose_game_source(
	const DTTR_LoaderUIDiscCandidate *disc_candidates,
	size_t disc_candidate_count
) {
	DTTR_ImGuiDialogContext ctx;

	if (!dttr_imgui_dialog_begin(
			&ctx,
			S_WINDOW_TITLE,
			DTTR_LOADER_UI_WINDOW_W,
			DTTR_LOADER_UI_WINDOW_H
		)) {
		return s_native_choose_game_source();
	}

	if (!disc_candidates) {
		disc_candidate_count = 0;
	} else {
		disc_candidate_count = s_clamp_disc_candidate_count(disc_candidate_count);
	}

	DTTR_LoaderUIChoice result = DTTR_LOADER_UI_CHOICE_EXIT;
	bool running = true;
	while (running) {
		dttr_imgui_dialog_process_events(&running);
		dttr_imgui_dialog_refresh_scale(&ctx);
		dttr_imgui_dialog_new_frame();

		if (dttr_imgui_dialog_begin_root(&ctx, S_WINDOW_TITLE)) {
			dttr_imgui_dialog_draw_header(&ctx, S_HEADER_TITLE, DTTR_VERSION);
			igSeparator();
			dttr_imgui_dialog_draw_padded_text(
				&ctx,
				S_GAME_SOURCE_MESSAGE,
				DTTR_LOADER_UI_TEXT_PADDING_X,
				DTTR_LOADER_UI_TEXT_PADDING_Y
			);
			s_draw_source_buttons(
				&ctx,
				disc_candidates,
				disc_candidate_count,
				&result,
				&running
			);
			dttr_imgui_dialog_fit_window_to_content(&ctx, DTTR_LOADER_UI_WINDOW_W, 18.0f);
		}
		dttr_imgui_dialog_end_root();

		dttr_imgui_dialog_render(&ctx);
	}

	dttr_imgui_dialog_end(&ctx);
	return result;
}

void dttr_loader_ui_show_error(const char *title, const char *message) {
	const char *window_title = title ? title : S_ERROR_TITLE;
	const char *safe_message = message ? message : "";

	if (dttr_imgui_error_show(window_title, safe_message)) {
		return;
	}

	dttr_sdl_show_simple_message_box(
		SDL_MESSAGEBOX_ERROR,
		window_title,
		safe_message,
		NULL
	);
}
