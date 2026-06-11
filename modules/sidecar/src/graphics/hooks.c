#include <dttr_pcdogs.h>

#include <stdlib.h>
#include <windows.h>

#include <dttr_config.h>

#include "graphics_com_private.h"
#include "graphics_private.h"
#include "hooks_private.h"
#include "sidecar_private.h"
#include <dttr_log.h>

static DTTR_Graphics_COM_DirectDraw7 *graphics_hook_ddraw7;
static HWND graphics_hook_hwnd;

static const DTTR_PCDOGS_T_Patch_Spec graphics_byte_patches[] = {
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(
		true,
		"83 F8 ?? 7C ?? D9 43 ?? D8 1D ?? ?? ?? ?? DF E0 F6 C4 41 0F ?? ?? ?? ??",
		19,
		0xE9,
		0xBA,
		0x00,
		0x00,
		0x00,
		0x90
	),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(
		true,
		"8B 08 EB ?? A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 3B C1",
		17,
		0x90,
		0x90
	),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(
		true,
		"83 C1 14 4E 75 ?? A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 3B C1",
		19,
		0x90,
		0x90
	),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(
		true,
		"DB 44 24 30 D9 1F",
		-15,
		0xD9,
		0x1F,
		0x90,
		0x90,
		0x90
	),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(
		true,
		"DB 44 24 30 D9 1F",
		-10,
		0x90,
		0x90,
		0x90,
		0x90
	),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(
		true,
		"8D AE ?? ?? ?? ?? DB 44 24 30 D9 1F",
		10,
		0x90,
		0x90
	),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(
		true,
		"8D AE ?? ?? ?? ?? DB 44 24 30",
		6,
		0x90,
		0x90,
		0x90,
		0x90
	),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(
		true,
		"8B 54 24 18 89 44 24 30",
		-5,
		0xD9,
		0x5D,
		0x00,
		0x90,
		0x90
	),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(
		true,
		"8B 54 24 18 89 44 24 30",
		4,
		0x90,
		0x90,
		0x90,
		0x90
	),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(true, "83 C0 14 50 55 D9 5D 00", 5, 0x90, 0x90, 0x90),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(true, "52 DB 44 24 34", 1, 0x90, 0x90, 0x90, 0x90),
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(false, "53 8B 5C 24 14 55 33 C9 56 57 85 DB", 0, 0xC3),
};

static DTTR_Core_PatchGroup *graphics_import_thunk_patch_group;
static DTTR_Core_PatchGroup *graphics_byte_patch_group;

// Lazily creates the DirectDraw 7 facade that satisfies the game's renderer bootstrap.
static DTTR_Graphics_COM_DirectDraw7 *get_or_create_ddraw7() {
	if (!graphics_hook_ddraw7) {
		graphics_hook_ddraw7 = dttr_graphics_com_create_directdraw7();
	}

	return graphics_hook_ddraw7;
}

// Temporarily relaxes page protection so the DirectDraw callback can publish the facade
// pointer through the game-provided slot.
static void store_pointer(void **slot, void *value) {
	DWORD old = 0;
	const SIZE_T slot_size = sizeof(*slot);

	if (!VirtualProtect(slot, slot_size, PAGE_READWRITE, &old)) {
		DTTR_LOG_ERROR(
			"VirtualProtect failed for slot=%p error=%lu",
			slot,
			GetLastError()
		);
		return;
	}

	*slot = value;
	VirtualProtect(slot, slot_size, old, &old);
}

// Replaces DirectDrawCreateEx with the sidecar facade so the game renders through the
// selected backend.
HRESULT __stdcall dttr_hook_directdraw_create_ex_callback(
	DTTR_PCDOGS_T_Win32_GUID *guid,
	void **ddraw_out,
	DTTR_PCDOGS_T_Win32_GUID *iid,
	DTTR_PCDOGS_T_COM_IUnknown *outer
) {
	DTTR_Graphics_COM_DirectDraw7 *ddraw7 = get_or_create_ddraw7();

	if (!ddraw7) {
		return E_OUTOFMEMORY;
	}

	DTTR_Result result = DTTR_PCDOGS_D_D3D_CreateTextureSurface_DDrawObject->Write(
		(DTTR_PCDOGS_T_DDraw_IDirectDraw7 *)ddraw7
	);
	if (!DTTR_ResultOK(result)) {
		DTTR_LOG_ERROR(
			"Failed to publish DirectDraw object: %s",
			dttr_sidecar_result_detail(result)
		);
		return E_FAIL;
	}

	if (ddraw_out) {
		store_pointer(ddraw_out, ddraw7);
	}

	DTTR_LOG_DEBUG("DirectDrawCreateEx returning S_OK, vtbl=%p", ddraw7->vtbl);
	return S_OK;
}

// Reports a single compatible DirectDraw device to keep the game enumeration path on
// the sidecar renderer.
HRESULT __stdcall dttr_hook_directdraw_enumerate_ex_a_callback(
	DDraw_EnumCallbackExA lpCallback,
	LPVOID lpContext,
	DWORD dwFlags
) {
	DTTR_LOG_DEBUG(
		"DirectDrawEnumerateExA intercepted - callback=%p context=%p flags=0x%x",
		lpCallback,
		lpContext,
		dwFlags
	);

	if (!lpCallback) {
		return S_OK;
	}

	lpCallback(NULL, "DTTR Virtual Display", "display", lpContext, NULL);

	return S_OK;
}

// Initializes the sidecar renderer before DirectDraw callbacks hand it to the game.
bool dttr_graphics_hooks_init(const DTTR_Mods_Context *ctx) {
	graphics_hook_hwnd = dttr_graphics_init();

	if (!graphics_hook_hwnd) {
		DTTR_LOG_ERROR("Failed to initialize backend");
		return false;
	}

	if (!get_or_create_ddraw7()) {
		DTTR_LOG_ERROR("Failed to create DirectDraw translator");
		return false;
	}

	const DTTR_PCDOGS_T_Patch_Spec graphics_import_thunk_patches[] = {
		DTTR_PCDOGS_F_DDraw_CreateEx
			->PatchSpec(true, dttr_hook_directdraw_create_ex_callback, NULL),
		DTTR_PCDOGS_F_DDraw_EnumerateExA
			->PatchSpec(true, dttr_hook_directdraw_enumerate_ex_a_callback, NULL),
	};

	if (!dttr_sidecar_install_pcdogs_patch_group(
			ctx,
			"sidecar/graphics-import-thunk",
			graphics_import_thunk_patches,
			DTTR_ARRAY_COUNT(graphics_import_thunk_patches),
			&graphics_import_thunk_patch_group
		)) {
		return false;
	}

	if (dttr_config.vertex_precision == DTTR_VERTEX_PRECISION_SUBPIXEL) {
		if (!dttr_sidecar_install_pcdogs_patch_group(
				ctx,
				"sidecar/graphics-byte-patch",
				graphics_byte_patches,
				DTTR_ARRAY_COUNT(graphics_byte_patches),
				&graphics_byte_patch_group
			)) {
			return false;
		}
	}

	return true;
}

// Releases DirectDraw facade state after graphics patches are removed.
void dttr_graphics_hooks_cleanup(const DTTR_Mods_Context *ctx) {
	DTTR_Core_PatchGroupRelease(&graphics_byte_patch_group);
	DTTR_Core_PatchGroupRelease(&graphics_import_thunk_patch_group);

	if (!graphics_hook_ddraw7) {
		return;
	}

	free(graphics_hook_ddraw7);
	graphics_hook_ddraw7 = NULL;
}
