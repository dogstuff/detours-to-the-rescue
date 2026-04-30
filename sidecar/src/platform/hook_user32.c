#include "imports_internal.h"

DTTR_IMPORT_HOOK_DECL(dttr_import_user32, registerclassa, "USER32.dll!RegisterClassA")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, getmessagea, "USER32.dll!GetMessageA")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, translatemessage, "USER32.dll!TranslateMessage")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, peekmessagea, "USER32.dll!PeekMessageA")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, dispatchmessagea, "USER32.dll!DispatchMessageA")
DTTR_IMPORT_HOOK_DECL(
	dttr_import_user32,
	translateacceleratora,
	"USER32.dll!TranslateAcceleratorA"
)
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, loadicona, "USER32.dll!LoadIconA")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, releasecapture, "USER32.dll!ReleaseCapture")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, messageboxa, "USER32.dll!MessageBoxA")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, clienttoscreen, "USER32.dll!ClientToScreen")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, wsprintfa, "USER32.dll!wsprintfA")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, loadcursora, "USER32.dll!LoadCursorA")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, createwindowexa, "USER32.dll!CreateWindowExA")
DTTR_IMPORT_HOOK_DECL(
	dttr_import_user32,
	createacceleratortablea,
	"USER32.dll!CreateAcceleratorTableA"
)
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, adjustwindowrect, "USER32.dll!AdjustWindowRect")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, updatewindow, "USER32.dll!UpdateWindow")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, showcursor, "USER32.dll!ShowCursor")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, showwindow, "USER32.dll!ShowWindow")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, postquitmessage, "USER32.dll!PostQuitMessage")
DTTR_IMPORT_HOOK_DECL(
	dttr_import_user32,
	systemparametersinfoa,
	"USER32.dll!SystemParametersInfoA"
)
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, sendmessagea, "USER32.dll!SendMessageA")
DTTR_IMPORT_HOOK_DECL(dttr_import_user32, defwindowproca, "USER32.dll!DefWindowProcA")

static const DTTR_ImportHookSpec s_user32_hooks[] = {
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, registerclassa, "RegisterClassA"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("GetAsyncKeyState"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, getmessagea, "GetMessageA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, translatemessage, "TranslateMessage"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, peekmessagea, "PeekMessageA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, dispatchmessagea, "DispatchMessageA"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_user32,
		translateacceleratora,
		"TranslateAcceleratorA"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, loadicona, "LoadIconA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, releasecapture, "ReleaseCapture"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, messageboxa, "MessageBoxA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, clienttoscreen, "ClientToScreen"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, wsprintfa, "wsprintfA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, loadcursora, "LoadCursorA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, createwindowexa, "CreateWindowExA"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_user32,
		createacceleratortablea,
		"CreateAcceleratorTableA"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, adjustwindowrect, "AdjustWindowRect"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, updatewindow, "UpdateWindow"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, showcursor, "ShowCursor"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, showwindow, "ShowWindow"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, postquitmessage, "PostQuitMessage"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_user32,
		systemparametersinfoa,
		"SystemParametersInfoA"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, sendmessagea, "SendMessageA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_user32, defwindowproca, "DefWindowProcA")
};

void dttr_platform_hooks_user32_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"USER32.dll",
		s_user32_hooks,
		sizeof(s_user32_hooks) / sizeof(s_user32_hooks[0])
	);
}

void dttr_platform_hooks_user32_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_user32_hooks,
		sizeof(s_user32_hooks) / sizeof(s_user32_hooks[0])
	);
}
