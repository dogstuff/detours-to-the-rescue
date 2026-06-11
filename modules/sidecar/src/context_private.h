#ifndef DTTR_SIDECAR_CONTEXT_PRIVATE_H
#define DTTR_SIDECAR_CONTEXT_PRIVATE_H

#include <windows.h>

#include <dttr_mods.h>
#include <dttr_runtime.h>

// Captures module handles and APIs before callbacks expose sidecar state.
void dttr_sidecar_init_context(HMODULE game_module, HMODULE sidecar_module);

// Exposes the single sidecar context shared by hooks, mods, and runtime calls.
const DTTR_Mods_Context *dttr_sidecar_context();
const DTTR_Core_Context *dttr_sidecar_runtime_context();

#endif // DTTR_SIDECAR_CONTEXT_PRIVATE_H
