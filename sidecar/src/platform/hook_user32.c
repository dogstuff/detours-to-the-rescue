#include "imports_internal.h"

#define S_USER32_IMPORTS(X)                                                               \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  registerclassa,                                                                     \
	  "RegisterClassA",                                                                   \
	  "USER32.dll!RegisterClassA")                                                        \
	X(SKIP, dttr_import_user32, getasynckeystate, "GetAsyncKeyState", "GetAsyncKeyState") \
	X(TRACE, dttr_import_user32, getmessagea, "GetMessageA", "USER32.dll!GetMessageA")    \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  translatemessage,                                                                   \
	  "TranslateMessage",                                                                 \
	  "USER32.dll!TranslateMessage")                                                      \
	X(TRACE, dttr_import_user32, peekmessagea, "PeekMessageA", "USER32.dll!PeekMessageA") \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  dispatchmessagea,                                                                   \
	  "DispatchMessageA",                                                                 \
	  "USER32.dll!DispatchMessageA")                                                      \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  translateacceleratora,                                                              \
	  "TranslateAcceleratorA",                                                            \
	  "USER32.dll!TranslateAcceleratorA")                                                 \
	X(TRACE, dttr_import_user32, loadicona, "LoadIconA", "USER32.dll!LoadIconA")          \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  releasecapture,                                                                     \
	  "ReleaseCapture",                                                                   \
	  "USER32.dll!ReleaseCapture")                                                        \
	X(TRACE, dttr_import_user32, messageboxa, "MessageBoxA", "USER32.dll!MessageBoxA")    \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  clienttoscreen,                                                                     \
	  "ClientToScreen",                                                                   \
	  "USER32.dll!ClientToScreen")                                                        \
	X(TRACE, dttr_import_user32, wsprintfa, "wsprintfA", "USER32.dll!wsprintfA")          \
	X(TRACE, dttr_import_user32, loadcursora, "LoadCursorA", "USER32.dll!LoadCursorA")    \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  createwindowexa,                                                                    \
	  "CreateWindowExA",                                                                  \
	  "USER32.dll!CreateWindowExA")                                                       \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  createacceleratortablea,                                                            \
	  "CreateAcceleratorTableA",                                                          \
	  "USER32.dll!CreateAcceleratorTableA")                                               \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  adjustwindowrect,                                                                   \
	  "AdjustWindowRect",                                                                 \
	  "USER32.dll!AdjustWindowRect")                                                      \
	X(TRACE, dttr_import_user32, updatewindow, "UpdateWindow", "USER32.dll!UpdateWindow") \
	X(TRACE, dttr_import_user32, showcursor, "ShowCursor", "USER32.dll!ShowCursor")       \
	X(TRACE, dttr_import_user32, showwindow, "ShowWindow", "USER32.dll!ShowWindow")       \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  postquitmessage,                                                                    \
	  "PostQuitMessage",                                                                  \
	  "USER32.dll!PostQuitMessage")                                                       \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  systemparametersinfoa,                                                              \
	  "SystemParametersInfoA",                                                            \
	  "USER32.dll!SystemParametersInfoA")                                                 \
	X(TRACE, dttr_import_user32, sendmessagea, "SendMessageA", "USER32.dll!SendMessageA") \
	X(TRACE,                                                                              \
	  dttr_import_user32,                                                                 \
	  defwindowproca,                                                                     \
	  "DefWindowProcA",                                                                   \
	  "USER32.dll!DefWindowProcA")

S_USER32_IMPORTS(DTTR_IMPORT_ENTRY_DECL)

static const DTTR_ImportHookSpec s_user32_hooks[] = {
	S_USER32_IMPORTS(DTTR_IMPORT_ENTRY_SPEC)
};

void dttr_platform_hooks_user32_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"USER32.dll",
		s_user32_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_user32_hooks)
	);
}

void dttr_platform_hooks_user32_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_user32_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_user32_hooks)
	);
}

#undef S_USER32_IMPORTS
