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

static const char *const ERROR_TITLE = "DttR: Error";
static const char *const HEADER_TITLE = "102 Crashes: Traces to the Rescue!";
static const char *const CRASH_DETAILS_MARKER = "\n\nContextFlags=";
static const char *const STACK_TRACE_MARKER = "\n\nStack trace:";
static const char *const DUMP_MARKER = "\n\nDump written to:";
static const char *const REPORT_MARKER = "\n\nFeel free to report this error";

typedef struct {
	const char *text;
	const char *end;
} text_span;

typedef struct {
	const char *summary_end;
	const char *report_text;
	sds stack_trace;
} error_message;

static text_span span(const char *text, const char *end) {
	return (text_span){text, end};
}

static size_t span_len(text_span span) {
	return (size_t)(span.end - span.text);
}

static sds sdsnewspan(text_span span) {
	return sdsnewlen(span.text, span_len(span));
}

static float text_padding_x(const DTTR_ImGuiDialogContext *ctx) {
	return DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_ERROR_UI_TEXT_PADDING_X);
}

static void set_text_padding_x(const DTTR_ImGuiDialogContext *ctx) {
	igSetCursorPosX(text_padding_x(ctx));
}

static bool begin_error_panel(const DTTR_ImGuiDialogContext *ctx) {
	return DTTR_ImGuiDialog_BeginPaddedPanel(
		ctx,
		"##error_panel",
		ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
	);
}

static error_message parse_error_message(const char *message) {
	const char *stack = strstr(message, STACK_TRACE_MARKER);
	if (!stack) {
		return (error_message){0};
	}

	const char *details = stack;
	const char *crash_details = strstr(message, CRASH_DETAILS_MARKER);
	if (crash_details && crash_details < stack) {
		details = crash_details;
	}

	const char *details_text = details + 2;
	const char *report_text = strstr(details_text, REPORT_MARKER);

	return (error_message){
		.summary_end = details,
		.report_text = report_text,
		.stack_trace = sdsnewlen(
			details_text,
			report_text ? (size_t)(report_text - details_text) : strlen(details_text)
		),
	};
}

static void draw_wrapped_text_span(const DTTR_ImGuiDialogContext *ctx, text_span text) {
	set_text_padding_x(ctx);
	igPushTextWrapPos(igGetWindowWidth() - text_padding_x(ctx));
	igTextWrapped("%.*s", (int)span_len(text), text.text);
	igPopTextWrapPos();
}

static void draw_wrapped_text(const DTTR_ImGuiDialogContext *ctx, const char *text) {
	draw_wrapped_text_span(ctx, span(text, text + strlen(text)));
}

static void draw_clickable_text(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *url
) {
	set_text_padding_x(ctx);
	igPushStyleColor_Vec4(ImGuiCol_TextLink, DTTR_IMGUI_COLOR_LINK);
	igTextLinkOpenURL(label, url);
	igPopStyleColor(1);
}

static sds file_url_for_parent_dir(const char *path) {
	const char *last_separator = NULL;
	for (const char *p = path; *p; p++) {
		if (DTTR_Path_IsSeparator(*p)) {
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

static void draw_dump_text(const DTTR_ImGuiDialogContext *ctx, text_span dump_text) {
	const char *newline = memchr(dump_text.text, '\n', span_len(dump_text));
	if (!newline || newline + 1 >= dump_text.end) {
		draw_wrapped_text_span(ctx, dump_text);
		return;
	}

	sds label = sdsnewspan(span(newline + 1, dump_text.end));
	if (label) {
		sdstrim(label, "\r\n");
	}

	sds url = label ? file_url_for_parent_dir(label) : NULL;
	if (!label || sdslen(label) == 0 || !url) {
		sdsfree(label);
		sdsfree(url);
		draw_wrapped_text_span(ctx, dump_text);
		return;
	}

	draw_wrapped_text_span(ctx, span(dump_text.text, newline));
	draw_clickable_text(ctx, label, url);
	sdsfree(label);
	sdsfree(url);
}

static void draw_report_text(const DTTR_ImGuiDialogContext *ctx, const char *report_text) {
	const char *text = report_text + 2;
	const char *url = strchr(text, '\n');
	if (!url || !url[1]) {
		draw_wrapped_text(ctx, text);
		return;
	}

	draw_wrapped_text_span(ctx, span(text, url));
	text_span url_text = span(url + 1, url + 1 + strcspn(url + 1, "\r\n"));
	sds url_copy = sdsnewspan(url_text);
	if (!url_copy || sdslen(url_copy) == 0) {
		sdsfree(url_copy);
		draw_wrapped_text(ctx, text);
		return;
	}

	draw_clickable_text(ctx, url_copy, url_copy);
	sdsfree(url_copy);
}

static const char *find_summary_dump(const error_message *message, const char *summary) {
	const char *dump_text = strstr(summary, DUMP_MARKER);
	return dump_text && dump_text < message->summary_end ? dump_text : NULL;
}

static void draw_copyable_stack_trace(
	const DTTR_ImGuiDialogContext *ctx,
	const char *summary,
	const error_message *message
) {
	const char *dump_text = find_summary_dump(message, summary);

	DTTR_ImGuiDialog_OffsetCursorY(ctx, DTTR_ERROR_UI_TEXT_PADDING_Y);

	if (message->report_text) {
		draw_report_text(ctx, message->report_text);
		DTTR_ImGuiDialog_OffsetCursorY(ctx, 8.0f);
	}

	set_text_padding_x(ctx);
	const float stack_width = igGetWindowWidth() - (text_padding_x(ctx) * 2.0f);
	igPushStyleColor_Vec4(ImGuiCol_FrameBg, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
	igPushStyleColor_Vec4(ImGuiCol_FrameBgHovered, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
	igPushStyleColor_Vec4(ImGuiCol_FrameBgActive, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
	igInputTextMultiline(
		"##stack_trace",
		message->stack_trace,
		sdslen(message->stack_trace) + 1,
		(
			ImVec2_c
		){stack_width, DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_ERROR_UI_STACK_BOX_H)},
		ImGuiInputTextFlags_ReadOnly,
		NULL,
		NULL
	);
	igPopStyleColor(3);

	if (dump_text) {
		DTTR_ImGuiDialog_OffsetCursorY(ctx, 8.0f);
		draw_dump_text(ctx, span(dump_text + 2, message->summary_end));
	}

	DTTR_ImGuiDialog_OffsetCursorY(ctx, DTTR_ERROR_UI_TEXT_PADDING_Y);
}

bool DTTR_ImGui_ErrorShow(const char *title, const char *message) {
	const char *window_title = title ? title : ERROR_TITLE;
	const char *safe_message = message ? message : "";
	error_message parsed_message = parse_error_message(safe_message);

	DTTR_ImGuiDialogContext ctx;
	if (!DTTR_ImGuiDialog_Begin(
			&ctx,
			window_title,
			DTTR_ERROR_UI_WINDOW_W,
			DTTR_ERROR_UI_INITIAL_WINDOW_H
		)) {
		sdsfree(parsed_message.stack_trace);
		return false;
	}

	bool running = true;
	while (running) {
		DTTR_ImGuiDialog_ProcessEvents(&ctx, &running);
		DTTR_ImGuiDialog_RefreshScale(&ctx);
		DTTR_ImGuiDialog_NewFrame(&ctx);

		DTTR_ImGuiDialog_PushTheme();
		if (DTTR_ImGuiDialog_BeginRoot(&ctx, window_title, ImGuiWindowFlags_None)) {
			if (begin_error_panel(&ctx)) {
				const ImVec2_c ok_button_size = {
					DTTR_ImGuiDialog_ScaledFloat(&ctx, 100.0f),
					DTTR_ImGuiDialog_ScaledFloat(&ctx, DTTR_ERROR_UI_BUTTON_H),
				};

				DTTR_ImGuiDialog_DrawHeader(&ctx, HEADER_TITLE, DTTR_VERSION);
				igSeparator();
				if (parsed_message.stack_trace) {
					draw_copyable_stack_trace(&ctx, safe_message, &parsed_message);
				} else {
					DTTR_ImGuiDialog_DrawPaddedText(
						&ctx,
						safe_message,
						DTTR_ERROR_UI_TEXT_PADDING_X,
						DTTR_ERROR_UI_TEXT_PADDING_Y
					);
				}

				DTTR_ImGuiDialog_CenterNextItem(ok_button_size.x);
				if (DTTR_ImGuiDialog_Button(&ctx, "##ok", "OK", ok_button_size)) {
					running = false;
				}

				igDummy((ImVec2_c){
					0.0f,
					DTTR_ImGuiDialog_ScaledFloat(&ctx, DTTR_ERROR_UI_TEXT_PADDING_Y),
				});
			}

			DTTR_ImGuiDialog_EndPaddedPanel();
			DTTR_ImGuiDialog_FitWindowToContent(&ctx, DTTR_ERROR_UI_WINDOW_W, 0.0f);
		}

		DTTR_ImGuiDialog_EndRoot();
		DTTR_ImGuiDialog_PopTheme();
		DTTR_ImGuiDialog_Render(&ctx);
	}

	DTTR_ImGuiDialog_End(&ctx);
	sdsfree(parsed_message.stack_trace);
	return true;
}
