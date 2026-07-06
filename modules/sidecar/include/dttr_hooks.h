#ifndef DTTR_HOOKS_H
#define DTTR_HOOKS_H

#include <stdint.h>
#include <windows.h>

/// Replaces the patched game Window_RunWinMain routine.
int32_t _stdcall DTTR_Hook_WinMainCallback(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int32_t nCmdShow
);

#endif
