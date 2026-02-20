#ifndef DTTR_HOOKS_GRAPHICS_H
#define DTTR_HOOKS_GRAPHICS_H

#include <dttr_interop.h>
#include <stdint.h>
#include <windows.h>

/// Installs DirectDraw-related runtime hooks
void dttr_graphics_hook_init(const DTTR_ComponentContext *ctx);
/// Removes DirectDraw-related runtime hooks and releases hook state
void dttr_graphics_hook_cleanup(const DTTR_ComponentContext *ctx);

/// Intercepts DirectDrawCreateEx and returns the sidecar DirectDraw translator
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreateex
HRESULT __stdcall dttr_graphics_hook_directdraw_create_ex_callback(
	const void *guid,
	void **ddraw_out,
	const void *iid,
	const void *outer
);

typedef BOOL(__stdcall *LPDDENUMCALLBACKEXA)(
	GUID *lpGUID,
	LPSTR lpDriverDescription,
	LPSTR lpDriverName,
	LPVOID lpContext,
	HMONITOR hm
);

/// Intercepts DirectDrawEnumerateExA and enumerates the virtual display adapter
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawenumerateexa
HRESULT __stdcall dttr_graphics_hook_directdraw_enumerate_ex_a_callback(
	LPDDENUMCALLBACKEXA lpCallback,
	LPVOID lpContext,
	DWORD dwFlags
);

// Sig matches call site; resolves through import thunk to IAT entry
DTTR_INTEROP_PATCH_PTR_SIG(
	dttr_hook_directdraw_create_ex,
	"\xE8????\x85\xC0\x7D?\x68????\x6A\x00\x50\xE8",
	"x????xxx?x????xxxx",
	DTTR_FF25_ADDR(DTTR_E8_TARGET(match)),
	dttr_graphics_hook_directdraw_create_ex_callback
)

// Sig matches call site; resolves through import thunk to IAT entry
DTTR_INTEROP_PATCH_PTR_SIG(
	dttr_hook_directdraw_enumerate_ex_a,
	"\xE8????\x8B\xF0\xA1",
	"x????xxx",
	DTTR_FF25_ADDR(DTTR_E8_TARGET(match)),
	dttr_graphics_hook_directdraw_enumerate_ex_a_callback
)

// clang-format off

// Disables pre-computed vertex coordinates by replacing JNE with JMP + NOP.
// cmp eax, ?; jl ?; fld [ebx+?]; fcomp [addr]; fnstsw ax; test ah, 0x41; jne ????
DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_fast_path,
	"\x83\xF8?\x7C?\xD9\x43?\xD8\x1D????\xDF\xE0\xF6\xC4\x41\x0F\x85????",
	"xx?x?xx?xx????xxxxxx????",
	match + 19, 6
)

// NOPs two JGE short jumps to remove the 1000-primitive batch limit.
// mov ecx, [eax]; jmp ?; mov eax, [g_var]; mov ecx, [g_var]; cmp eax, ecx
DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_batch_limit_a,
	"\x8B\x08\xEB?\xA1????\x8B\x0D????\x3B\xC1",
	"xxx?x????xx????xx",
	match + 17, 2
)

// add ecx, 0x14; dec esi; jne ?; mov eax, [g_var]; mov ecx, [g_var]; cmp eax, ecx
DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_batch_limit_b,
	"\x83\xC1\x14\x4E\x75?\xA1????\x8B\x0D????\x3B\xC1",
	"xxxxx?x????xx????xx",
	match + 19, 2
)

// Replaces the X projection call to __ftol with fstp dword [edi] and NOPs the mov/fild/fstp.
// fild dword [esp+0x30]; fstp dword [edi]
DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_ftol_x,
	"\xDB\x44\x24\x30\xD9\x1F",
	"xxxxxx",
	match - 15, 5
)

DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_mov_x,
	"\xDB\x44\x24\x30\xD9\x1F",
	"xxxxxx",
	match - 10, 4
)
// Uses the lea gap as anchor since fild_x will NOP the fild bytes. Applied before fild_x.
// lea ebp,[esi+??]; fild [esp+0x30]; fstp [edi]
DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_fstp2_x,
	"\x8D\xAE????\xDB\x44\x24\x30\xD9\x1F",
	"xx????xxxxxx",
	match + 10, 2
)

// Shorter anchor excluding the D9 1F that fstp2_x already NOPed.
// lea ebp,[esi+??]; fild [esp+0x30]
DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_fild_x,
	"\x8D\xAE????\xDB\x44\x24\x30",
	"xx????xxxx",
	match + 6, 4
)

// Replaces the Y projection call to __ftol with fstp dword [ebp+0] and NOPs the mov/fild/fstp.
// mov edx, [esp+0x18]; mov [esp+0x30], eax
DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_ftol_y,
	"\x8B\x54\x24\x18\x89\x44\x24\x30",
	"xxxxxxxx",
	match - 5, 5
)

DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_mov_y,
	"\x8B\x54\x24\x18\x89\x44\x24\x30",
	"xxxxxxxx",
	match + 4, 4
)

// Uses the add/push gap as anchor since fild_y will NOP mov_y's anchor. Applied before fild_y.
// add eax,0x14; push eax; push ebp; fstp [ebp+0]
DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_fstp2_y,
	"\x83\xC0\x14\x50\x55\xD9\x5D\x00",
	"xxxxxxxx",
	match + 5, 3
)

// Anchored in the push/fild gap which is unaffected by prior patches.
// push edx; fild [esp+0x34]
DTTR_INTEROP_PATCH_BYTES_SIG(
	dttr_hook_precision_fild_y,
	"\x52\xDB\x44\x24\x34",
	"xxxxx",
	match + 1, 4
)

// Disables Math_SnapVertexToNearestPoint which snaps to int16 positions.
DTTR_INTEROP_PATCH_BYTES_OPTIONAL_SIG(
	dttr_hook_render_quad_snap,
	"\x53\x8b\x5c\x24\x14\x55\x33\xc9\x56\x57\x85\xdb",
	"xxxxxxxxxxxx",
	match, 1
)

// Patches the early X culling bound imm32 in mov eax, 639.
DTTR_INTEROP_PATCH_BYTES_OPTIONAL_SIG(
	dttr_hook_render_quad_cull_x,
	"\xb8\x7f\x02\x00\x00",
	"xxxxx",
	match + 1, 4
)

// Patches the early Y culling bound imm32 in mov eax, 479.
DTTR_INTEROP_PATCH_BYTES_OPTIONAL_SIG(
	dttr_hook_render_quad_cull_y,
	"\xb8\xdf\x01\x00\x00",
	"xxxxx",
	match + 1, 4
)

// NOPs the X min clamp mov dword [edi], 0x0 (6 bytes at offset +5).
DTTR_INTEROP_PATCH_BYTES_OPTIONAL_SIG(
	dttr_hook_render_quad_clamp_x_min,
	"\xf6\xc4\x01\x74\x06\xc7\x07\x00\x00\x00\x00\xd9\x07",
	"xxxxxxxxxxxxx",
	match + 5, 6
)

// Patches the X max clamp from mov dword [edi], 639.0f to mov dword [edi], float(w-1).
DTTR_INTEROP_PATCH_BYTES_OPTIONAL_SIG(
	dttr_hook_render_quad_clamp_x_max,
	"\xc7\x07\x00\xc0\x1f\x44",
	"xxxxxx",
	match, 6
)

// NOPs the Y min clamp mov dword [ebp+0], 0x0 (7 bytes).
DTTR_INTEROP_PATCH_BYTES_OPTIONAL_SIG(
	dttr_hook_render_quad_clamp_y_min,
	"\xc7\x45\x00\x00\x00\x00\x00",
	"xxxxxxx",
	match, 7
)

// Patches the Y max clamp from mov dword [ebp+0], 479.0f to mov dword [ebp+0], float(h-1).
DTTR_INTEROP_PATCH_BYTES_OPTIONAL_SIG(
	dttr_hook_render_quad_clamp_y_max,
	"\xc7\x45\x00\x00\x80\xef\x43",
	"xxxxxxx",
	match, 7
)

// clang-format on

#endif /* DTTR_HOOKS_GRAPHICS_H */
