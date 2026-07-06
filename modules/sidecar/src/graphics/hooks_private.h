#ifndef DTTR_GRAPHICS_HOOKS_PRIVATE_H
#define DTTR_GRAPHICS_HOOKS_PRIVATE_H

#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <windows.h>

// Creates the sidecar graphics wrapper for DirectDraw.
HRESULT __stdcall dttr_hook_directdraw_create_ex_callback(
	DTTR_PCDOGS_T_Win32_GUID *guid,
	void **ddraw_out,
	DTTR_PCDOGS_T_Win32_GUID *iid,
	DTTR_PCDOGS_T_COM_IUnknown *outer
);

// This typedef names the DirectDraw enumeration callback used by the hook group.
typedef BOOL(__stdcall *DDraw_EnumCallbackExA)(
	GUID *lpGUID,
	LPSTR lpDriverDescription,
	LPSTR lpDriverName,
	LPVOID lpContext,
	HMONITOR hm
);

// Enumerates the sidecar graphics driver through DirectDraw.
HRESULT __stdcall dttr_hook_directdraw_enumerate_ex_a_callback(
	DDraw_EnumCallbackExA lpCallback,
	LPVOID lpContext,
	DWORD dwFlags
);

// This declaration installs the graphics hook group.
bool dttr_graphics_hooks_init(const DTTR_Mods_Context *ctx);

// This declaration releases the graphics hook group.
void dttr_graphics_hooks_cleanup(const DTTR_Mods_Context *ctx);

#endif // DTTR_GRAPHICS_HOOKS_PRIVATE_H
