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

static const char *const WINDOW_TITLE = "DttR: Specify Game Files";
static const char *const HEADER_TITLE = "102 Patches: Detours to the Rescue!";
static const char *const GAME_SOURCE_MESSAGE
	= "Select either a directory containing the 102 Dalmatians: Puppies to the Rescue "
	  "files, the original game disc, or an ISO image.";
static const char *const ERROR_TITLE = "DttR: Error";

static DTTR_LoaderUIChoice native_choose_game_source() {
	const SDL_MessageBoxButtonData buttons[] = {
		{0, DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER, "Open Directory"},
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		 DTTR_LOADER_UI_CHOICE_BROWSE_ISO,
		 "Open ISO"},
	};

	const SDL_MessageBoxData msgbox = {
		.flags = SDL_MESSAGEBOX_INFORMATION,
		.window = NULL,
		.title = WINDOW_TITLE,
		.message = GAME_SOURCE_MESSAGE,
		.numbuttons = 2,
		.buttons = buttons,
	};

	int choice_id = DTTR_LOADER_UI_CHOICE_EXIT;
	if (!DTTR_SDL_ShowMessageBox(&msgbox, &choice_id)) {
		return DTTR_LOADER_UI_CHOICE_EXIT;
	}

	return DTTR_LoaderUI_ChoiceFromID(choice_id);
}

static float button_spacing(const DTTR_ImGuiDialogContext *ctx) {
	return DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_LOADER_UI_BUTTON_SPACING);
}

static float button_row_width(const DTTR_ImGuiDialogContext *ctx, size_t button_count) {
	if (button_count == 0) {
		return 0.0f;
	}

	const float button_width = DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_LOADER_UI_BUTTON_W);
	return (button_width * (float)button_count)
		   + (button_spacing(ctx) * (float)(button_count - 1));
}

static ImVec2_c button_size(const DTTR_ImGuiDialogContext *ctx) {
	return (ImVec2_c){
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_LOADER_UI_BUTTON_W),
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_LOADER_UI_BUTTON_H),
	};
}

static bool draw_source_button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	DTTR_LoaderUIChoice choice,
	DTTR_LoaderUIChoice *result,
	bool *running
) {
	if (!DTTR_ImGuiDialog_Button(ctx, id, label, button_size(ctx))) {
		return false;
	}

	*result = choice;
	*running = false;
	return true;
}

static bool draw_disc_button(
	const DTTR_ImGuiDialogContext *ctx,
	size_t disc_index,
	const DTTR_LoaderUIDiscCandidate *disc_candidates,
	DTTR_LoaderUIChoice *result,
	bool *running
) {
	igPushID_Int((int)disc_index);
	const bool selected = draw_source_button(
		ctx,
		"##disc",
		disc_candidates[disc_index].label,
		DTTR_LoaderUI_DiscChoiceForIndex(disc_index),
		result,
		running
	);
	igPopID();
	return selected;
}

static void same_button_row(const DTTR_ImGuiDialogContext *ctx) {
	igSameLine(0.0f, button_spacing(ctx));
}

static size_t clamp_disc_candidate_count(size_t disc_candidate_count) {
	if (disc_candidate_count > DTTR_LOADER_UI_MAX_DISC_CANDIDATES) {
		return DTTR_LOADER_UI_MAX_DISC_CANDIDATES;
	}

	return disc_candidate_count;
}

static void draw_browse_buttons(
	const DTTR_ImGuiDialogContext *ctx,
	DTTR_LoaderUIChoice *result,
	bool *running
) {
	if (draw_source_button(
			ctx,
			"##open_directory",
			"Open Directory",
			DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER,
			result,
			running
		)) {
		return;
	}

	same_button_row(ctx);
	draw_source_button(
		ctx,
		"##open_iso",
		"Open ISO",
		DTTR_LOADER_UI_CHOICE_BROWSE_ISO,
		result,
		running
	);
}

static void draw_source_buttons(
	const DTTR_ImGuiDialogContext *ctx,
	const DTTR_LoaderUIDiscCandidate *disc_candidates,
	size_t disc_candidate_count,
	DTTR_LoaderUIChoice *result,
	bool *running
) {
	size_t disc_index = 0;
	const size_t first_row_buttons = disc_candidate_count > 0 ? 3 : 2;
	DTTR_ImGuiDialog_CenterNextItem(button_row_width(ctx, first_row_buttons));

	if (disc_candidate_count > 0) {
		draw_disc_button(ctx, disc_index++, disc_candidates, result, running);
		same_button_row(ctx);
	}

	draw_browse_buttons(ctx, result, running);

	while (disc_index < disc_candidate_count && *running) {
		const size_t remaining = disc_candidate_count - disc_index;
		const size_t row_count = remaining < 3 ? remaining : 3;
		DTTR_ImGuiDialog_CenterNextItem(button_row_width(ctx, row_count));

		for (size_t col = 0; col < row_count && *running; col++, disc_index++) {
			if (col > 0) {
				same_button_row(ctx);
			}

			draw_disc_button(ctx, disc_index, disc_candidates, result, running);
		}
	}
}

DTTR_LoaderUIChoice DTTR_LoaderUI_ChooseGameSource(
	const DTTR_LoaderUIDiscCandidate *disc_candidates,
	size_t disc_candidate_count
) {
	DTTR_ImGuiDialogContext ctx = {0};

	if (!DTTR_ImGuiDialog_Begin(
			&ctx,
			WINDOW_TITLE,
			DTTR_LOADER_UI_WINDOW_W,
			DTTR_LOADER_UI_WINDOW_H
		)) {
		return native_choose_game_source();
	}

	disc_candidate_count = disc_candidates
							   ? clamp_disc_candidate_count(disc_candidate_count)
							   : 0;

	DTTR_LoaderUIChoice result = DTTR_LOADER_UI_CHOICE_EXIT;
	bool running = true;
	while (running) {
		DTTR_ImGuiDialog_ProcessEvents(&ctx, &running);
		DTTR_ImGuiDialog_RefreshScale(&ctx);
		DTTR_ImGuiDialog_NewFrame(&ctx);

		if (DTTR_ImGuiDialog_BeginRoot(&ctx, WINDOW_TITLE, ImGuiWindowFlags_None)) {
			DTTR_ImGuiDialog_DrawHeader(&ctx, HEADER_TITLE, DTTR_VERSION);
			igSeparator();
			DTTR_ImGuiDialog_DrawPaddedText(
				&ctx,
				GAME_SOURCE_MESSAGE,
				DTTR_LOADER_UI_TEXT_PADDING_X,
				DTTR_LOADER_UI_TEXT_PADDING_Y
			);
			draw_source_buttons(
				&ctx,
				disc_candidates,
				disc_candidate_count,
				&result,
				&running
			);
			DTTR_ImGuiDialog_FitWindowToContent(&ctx, DTTR_LOADER_UI_WINDOW_W, 18.0f);
		}

		DTTR_ImGuiDialog_EndRoot();

		DTTR_ImGuiDialog_Render(&ctx);
	}

	DTTR_ImGuiDialog_End(&ctx);
	return result;
}

void DTTR_LoaderUI_ShowError(const char *title, const char *message) {
	const char *window_title = title ? title : ERROR_TITLE;
	const char *safe_message = message ? message : "";

	if (DTTR_ImGui_ErrorShow(window_title, safe_message)) {
		return;
	}

	DTTR_SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, window_title, safe_message, NULL);
}
