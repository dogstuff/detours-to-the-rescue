#ifndef DTTR_SIDECAR_HOOK_SIGS_H
#define DTTR_SIDECAR_HOOK_SIGS_H

// Single source of truth for the sidecar-owned PCDOGS hook signatures that are NOT SDK
// blueprint symbols. The runtime (entrypoint/inputs/game hooks) and the sidecar pcdogs
// test both reference these AOB strings, so a signature only ever needs editing here.
//
// The contiguous hook tables (graphics byte patches, mss32 imports) live in their own
// X-macro `.def` files alongside this header.

// Window_RunWinMain bootstrap hook installed from DllMain before the mod context exists.
#define DTTR_SIDECAR_AOB_WIN_MAIN "83 EC 40 53 8B 5C 24"

// DirectInput poll entry wrapped to feed SDL gamepad state into the game.
#define DTTR_SIDECAR_AOB_DINPUT_POLL "56 8B 74 24 ?? 56 8B 06"

// PCDOGS save-path resolver hooked to redirect persisted data (EU/SC builds only).
#define DTTR_SIDECAR_AOB_RESOLVE_PCDOGS_PATH "51 8D 44 24 ?? 57"

// Expands a parenthesized byte list `(0xAA, 0xBB)` into its bare comma-separated form so it
// can feed variadic patch-spec/compound-literal macros from a single `.def` field.
#define DTTR_SIDECAR_UNPAREN(...) __VA_ARGS__

#endif // DTTR_SIDECAR_HOOK_SIGS_H
