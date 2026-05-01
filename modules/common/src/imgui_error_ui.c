#include <dttr_imgui.h>
#include <dttr_path.h>

#include <sds.h>
#include <string.h>

#define DTTR_ERROR_UI_WINDOW_W 560
#define DTTR_ERROR_UI_INITIAL_WINDOW_H 120
#define DTTR_ERROR_UI_BUTTON_H 28.0f
#define DTTR_ERROR_UI_TEXT_PADDING_X 18.0f
#define DTTR_ERROR_UI_TEXT_PADDING_Y 16.0f
#define DTTR_ERROR_UI_STACK_BOX_H 150.0f

#ifndef DTTR_VERSION
#define DTTR_VERSION "unknown"
#endif

static const char *const S_ERROR_TITLE = "DttR: Error";
static const char *const S_HEADER_TITLE = "102 Crashes: Traces to the Rescue!";
static const char *const S_STACK_TRACE_MARKER = "\n\nStack trace:";
static const char *const S_DUMP_MARKER = "\n\nDump written to:";
static const char *const S_REPORT_MARKER = "\n\nIf this error is unexpected";

typedef struct {
	const char *m_text;
	const char *m_end;
} S_TextSpan;

typedef struct {
	const char *m_summary_end;
	const char *m_report_text;
	sds m_stack_trace;
} S_ErrorMessage;

static S_TextSpan s_span(const char *text, const char *end) {
	return (S_TextSpan){text, end};
}

static size_t s_span_len(S_TextSpan span) { return (size_t)(span.m_end - span.m_text); }

static sds s_sdsnewspan(S_TextSpan span) {
	return sdsnewlen(span.m_text, s_span_len(span));
}

static S_ErrorMessage s_parse_error_message(const char *message) {
	const char *stack = strstr(message, S_STACK_TRACE_MARKER);
	if (!stack) {
		return (S_ErrorMessage){0};
	}

	const char *stack_text = stack + 2;
	const char *report_text = strstr(stack_text, S_REPORT_MARKER);

	return (S_ErrorMessage){
		.m_summary_end = stack,
		.m_report_text = report_text,
		.m_stack_trace = sdsnewlen(
			stack_text,
			report_text ? (size_t)(report_text - stack_text) : strlen(stack_text)
		),
	};
}

static void s_draw_wrapped_text_span(const DTTR_ImGuiDialogContext *ctx, S_TextSpan text) {
	igSetCursorPosX(dttr_imgui_dialog_scaled_float(ctx, DTTR_ERROR_UI_TEXT_PADDING_X));
	igPushTextWrapPos(
		igGetWindowWidth()
		- dttr_imgui_dialog_scaled_float(ctx, DTTR_ERROR_UI_TEXT_PADDING_X)
	);
	igTextWrapped("%.*s", (int)s_span_len(text), text.m_text);
	igPopTextWrapPos();
}

static void s_draw_wrapped_text(const DTTR_ImGuiDialogContext *ctx, const char *text) {
	s_draw_wrapped_text_span(ctx, s_span(text, text + strlen(text)));
}

static void s_draw_clickable_text(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *url
) {
	igSetCursorPosX(dttr_imgui_dialog_scaled_float(ctx, DTTR_ERROR_UI_TEXT_PADDING_X));
	igTextColored(DTTR_IMGUI_COLOR_LINK, "%s", label);
	if (igIsItemHovered(ImGuiHoveredFlags_None)) {
		igSetMouseCursor(ImGuiMouseCursor_Hand);
	}
	if (igIsItemClicked(ImGuiMouseButton_Left)) {
		SDL_OpenURL(url);
	}
}

static sds s_file_url_for_parent_dir(const char *path) {
	const char *last_separator = NULL;
	for (const char *p = path; *p; p++) {
		if (dttr_path_is_separator(*p)) {
			last_separator = p;
		}
	}
	if (!last_separator) {
		return NULL;
	}

	sds url = sdsnew("file:///");
	for (const char *p = path; url && p < last_separator; p++) {
		if (*p == '\\') {
			url = sdscatlen(url, "/", 1);
		} else if (*p == ' ') {
			url = sdscat(url, "%20");
		} else {
			url = sdscatlen(url, p, 1);
		}
	}
	return url;
}

static void s_add_vertical_spacing(const DTTR_ImGuiDialogContext *ctx, float amount) {
	igSetCursorPosY(igGetCursorPosY() + dttr_imgui_dialog_scaled_float(ctx, amount));
}

static void s_draw_dump_text(const DTTR_ImGuiDialogContext *ctx, S_TextSpan dump_text) {
	const char *newline = memchr(dump_text.m_text, '\n', s_span_len(dump_text));
	if (!newline || newline + 1 >= dump_text.m_end) {
		s_draw_wrapped_text_span(ctx, dump_text);
		return;
	}

	s_draw_wrapped_text_span(ctx, s_span(dump_text.m_text, newline));
	sds label = s_sdsnewspan(s_span(newline + 1, dump_text.m_end));
	if (label) {
		sdstrim(label, "\r\n");
	}
	sds url = label ? s_file_url_for_parent_dir(label) : NULL;
	if (!label || sdslen(label) == 0 || !url) {
		sdsfree(label);
		sdsfree(url);
		s_draw_wrapped_text_span(ctx, dump_text);
		return;
	}

	s_draw_clickable_text(ctx, label, url);
	sdsfree(label);
	sdsfree(url);
}

static void s_draw_report_text(
	const DTTR_ImGuiDialogContext *ctx,
	const char *report_text
) {
	const char *text = report_text + 2;
	const char *url = strchr(text, '\n');
	if (!url || !url[1]) {
		s_draw_wrapped_text(ctx, text);
		return;
	}

	s_draw_wrapped_text_span(ctx, s_span(text, url));
	S_TextSpan url_text = s_span(url + 1, url + 1 + strcspn(url + 1, "\r\n"));
	sds url_copy = s_sdsnewspan(url_text);
	if (!url_copy || sdslen(url_copy) == 0) {
		sdsfree(url_copy);
		s_draw_wrapped_text(ctx, text);
		return;
	}

	s_draw_clickable_text(ctx, url_copy, url_copy);
	sdsfree(url_copy);
}

static const char *s_find_summary_dump(const S_ErrorMessage *message, const char *summary) {
	const char *dump_text = strstr(summary, S_DUMP_MARKER);
	return dump_text && dump_text < message->m_summary_end ? dump_text : NULL;
}

static void s_draw_copyable_stack_trace(
	const DTTR_ImGuiDialogContext *ctx,
	const char *summary,
	const S_ErrorMessage *message
) {
	const char *dump_text = s_find_summary_dump(message, summary);

	s_add_vertical_spacing(ctx, DTTR_ERROR_UI_TEXT_PADDING_Y);

	if (message->m_report_text) {
		s_draw_report_text(ctx, message->m_report_text);
		s_add_vertical_spacing(ctx, 8.0f);
	}

	igSetCursorPosX(dttr_imgui_dialog_scaled_float(ctx, DTTR_ERROR_UI_TEXT_PADDING_X));
	const float stack_width = igGetWindowWidth()
							  - dttr_imgui_dialog_scaled_float(
								  ctx,
								  DTTR_ERROR_UI_TEXT_PADDING_X * 2.0f
							  );
	igPushStyleColor_Vec4(ImGuiCol_FrameBg, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
	igPushStyleColor_Vec4(ImGuiCol_FrameBgHovered, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
	igPushStyleColor_Vec4(ImGuiCol_FrameBgActive, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
	igInputTextMultiline(
		"##stack_trace",
		message->m_stack_trace,
		sdslen(message->m_stack_trace) + 1,
		(ImVec2_c){stack_width,
				   dttr_imgui_dialog_scaled_float(ctx, DTTR_ERROR_UI_STACK_BOX_H)},
		ImGuiInputTextFlags_ReadOnly,
		NULL,
		NULL
	);
	igPopStyleColor(3);

	if (dump_text) {
		s_add_vertical_spacing(ctx, 8.0f);
		s_draw_dump_text(ctx, s_span(dump_text + 2, message->m_summary_end));
	}

	s_add_vertical_spacing(ctx, DTTR_ERROR_UI_TEXT_PADDING_Y);
}

bool dttr_imgui_error_show(const char *title, const char *message) {
	const char *window_title = title ? title : S_ERROR_TITLE;
	const char *safe_message = message ? message : "";
	S_ErrorMessage parsed_message = s_parse_error_message(safe_message);

	DTTR_ImGuiDialogContext ctx;
	if (!dttr_imgui_dialog_begin(
			&ctx,
			window_title,
			DTTR_ERROR_UI_WINDOW_W,
			DTTR_ERROR_UI_INITIAL_WINDOW_H
		)) {
		sdsfree(parsed_message.m_stack_trace);
		return false;
	}

	bool running = true;
	while (running) {
		dttr_imgui_dialog_process_events(&running);
		dttr_imgui_dialog_refresh_scale(&ctx);
		dttr_imgui_dialog_new_frame();

		if (dttr_imgui_dialog_begin_root(&ctx, window_title)) {
			const ImVec2_c ok_button_size = {
				dttr_imgui_dialog_scaled_float(&ctx, 100.0f),
				dttr_imgui_dialog_scaled_float(&ctx, DTTR_ERROR_UI_BUTTON_H),
			};
			dttr_imgui_dialog_draw_header(&ctx, S_HEADER_TITLE, DTTR_VERSION);
			if (parsed_message.m_stack_trace) {
				s_draw_copyable_stack_trace(&ctx, safe_message, &parsed_message);
			} else {
				dttr_imgui_dialog_draw_padded_text(
					&ctx,
					safe_message,
					DTTR_ERROR_UI_TEXT_PADDING_X,
					DTTR_ERROR_UI_TEXT_PADDING_Y
				);
			}
			dttr_imgui_dialog_center_next_item(ok_button_size.x);
			if (dttr_imgui_dialog_button(&ctx, "##ok", "OK", ok_button_size)) {
				running = false;
			}
			igDummy((ImVec2_c){
				0.0f,
				dttr_imgui_dialog_scaled_float(&ctx, DTTR_ERROR_UI_TEXT_PADDING_Y),
			});
			dttr_imgui_dialog_fit_window_to_content(&ctx, DTTR_ERROR_UI_WINDOW_W, 0.0f);
		}

		dttr_imgui_dialog_end_root();
		dttr_imgui_dialog_render(&ctx);
	}

	dttr_imgui_dialog_end(&ctx);
	sdsfree(parsed_message.m_stack_trace);
	return true;
}
