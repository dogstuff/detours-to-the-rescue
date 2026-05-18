#ifndef DTTR_GRAPHICS_HOOKS_PRIVATE_H
#define DTTR_GRAPHICS_HOOKS_PRIVATE_H

#include <dttr_mods.h>
#include <windows.h>

// Installs graphics patches and initializes the selected renderer for the game.
bool dttr_graphics_hooks_init(const DTTR_Mods_Context *ctx);
// Releases graphics patches and renderer state during sidecar shutdown.
void dttr_graphics_hooks_cleanup(const DTTR_Mods_Context *ctx);

// Replacement DirectDrawCreateEx callback that returns the sidecar renderer facade.
HRESULT __stdcall dttr_hook_directdraw_create_ex_callback(
	GUID *guid,
	void **ddraw_out,
	GUID *iid,
	IUnknown *outer
);

typedef BOOL(__stdcall *DDraw_EnumCallbackExA)(
	GUID *lpGUID,
	LPSTR lpDriverDescription,
	LPSTR lpDriverName,
	LPVOID lpContext,
	HMONITOR hm
);

// Replacement DirectDraw enumeration callback that exposes the sidecar renderer facade.
HRESULT __stdcall dttr_hook_directdraw_enumerate_ex_a_callback(
	DDraw_EnumCallbackExA lpCallback,
	LPVOID lpContext,
	DWORD dwFlags
);

#endif // DTTR_GRAPHICS_HOOKS_PRIVATE_H
