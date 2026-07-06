#ifndef DTTR_SIDECAR_HOOK_SIGS_H
#define DTTR_SIDECAR_HOOK_SIGS_H

// Single source of truth for the sidecar-owned PCDOGS hook signatures that are NOT SDK
// blueprint symbols. The runtime (entrypoint/inputs/game hooks) and the sidecar pcdogs
// test both reference these AOB strings, so a signature only ever needs editing here.
//
// The contiguous hook tables (graphics byte patches, mss32 imports) live in their own
// X-macro `.def` files alongside this header.

// This signature locates the Window_RunWinMain bootstrap hook installed from DllMain.
#define DTTR_SIDECAR_AOB_WIN_MAIN "83 EC 40 53 8B 5C 24"

// This signature locates the DirectInput poll entry wrapped by SDL gamepad state.
#define DTTR_SIDECAR_AOB_DINPUT_POLL "56 8B 74 24 ?? 56 8B 06"

// The native game treats Enter as "stop remapping" before it can be bound.
// We remove this behavior in DttR.
#define DTTR_SIDECAR_AOB_CONTROLS_ENTER_BIND_BRANCH                                      \
	"E8 ?? ?? ?? ?? 8B F0 83 FE FF 0F 84 ?? ?? ?? ?? 83 FE 0D 0F 84 ?? ?? ?? ?? 83 FE "  \
	"1B"

// The native game accepts keyboard bindings below 0x100. DttR scancode and SDL
// controller button bindings live above that range, so the keyboard-column guards
// need to stop above the sidecar button range instead.
#define DTTR_SIDECAR_AOB_CONTROLS_KEYBOARD_BIND_LIMIT                                    \
	"3B C7 75 17 81 FE 00 01 00 00 7D 29 8B ?? ?? ?? ?? ?? 89 34"

// This signature locates the controls menu remap-completion limit check.
#define DTTR_SIDECAR_AOB_CONTROLS_REMAP_DONE_LIMIT                                       \
	"3B C7 75 10 81 FE 00 01 00 00 7D 15 89 3D ?? ?? ?? ?? EB 3A 83 F8 01 75 F3 81 "     \
	"FE 00 01 00 00 7D EB"

// This signature locates the PCDOGS save-path resolver used to redirect persisted data.
#define DTTR_SIDECAR_AOB_RESOLVE_PCDOGS_PATH "51 8D 44 24 ?? 57"

// This macro expands parenthesized byte lists for runtime patch specs and test literals.
#define DTTR_SIDECAR_UNPAREN(...) __VA_ARGS__

#endif // DTTR_SIDECAR_HOOK_SIGS_H
