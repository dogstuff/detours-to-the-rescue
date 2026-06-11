#include <dttr_pcdogs.h>

#include <stdlib.h>
#include <windows.h>

#include <dttr_config.h>

#include "graphics_com_private.h"
#include "graphics_private.h"
#include "hooks_private.h"
#include "sidecar_hook_sigs.h"
#include "sidecar_private.h"
#include <dttr_log.h>

static DTTR_Graphics_COM_DirectDraw7 *graphics_hook_ddraw7;
static HWND graphics_hook_hwnd;

// Expanded from the shared single source so the sidecar pcdogs test can assert these same
// patch sites without re-specifying the signatures. See sidecar_graphics_byte_patches.def.
#define SIDECAR_GFX_BYTE_PATCH(                                                          \
	name,                                                                                \
	rt_required,                                                                         \
	test_required,                                                                       \
	aob,                                                                                 \
	offset,                                                                              \
	patch_seq,                                                                           \
	original_seq,                                                                        \
	original_mask                                                                        \
)                                                                                        \
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(                                                    \
		rt_required,                                                                     \
		aob,                                                                             \
		offset,                                                                          \
		DTTR_SIDECAR_UNPAREN patch_seq                                                   \
	),

static const DTTR_PCDOGS_T_Patch_Spec graphics_byte_patches[] = {
#include "sidecar_graphics_byte_patches.def"
};
#undef SIDECAR_GFX_BYTE_PATCH

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

	if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_D3D_CreateTextureSurface_DDrawObject->Write(
			(DTTR_PCDOGS_T_DDraw_IDirectDraw7 *)ddraw7
		))) {
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
