#include "imports_internal.h"

#include <dttr_log.h>

#include <string.h>

int g_dttr_import_hook_trace_enabled;

void dttr_platform_hooks_ddraw_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_ddraw_cleanup(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_dinput_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_dinput_cleanup(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_gdi32_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_gdi32_cleanup(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_kernel32_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_kernel32_cleanup(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_user32_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_user32_cleanup(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_winmm_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_winmm_cleanup(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_mss32_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_mss32_cleanup(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_winplay_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_winplay_cleanup(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_winstr_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_winstr_cleanup(const DTTR_ComponentContext *ctx);

void dttr_import_hook_trace(const char *name) { DTTR_LOG_TRACE("Import call: %s", name); }

void dttr_import_hook_warn(const char *name) {
	DTTR_LOG_WARN("Unexpected import call: %s", name);
}

bool dttr_import_hook_install_spec(
	const DTTR_ComponentContext *ctx,
	const DTTR_ImportHookSpec *spec,
	uintptr_t site
) {
	*spec->m_site = site;
	*spec->m_handle = ctx->m_game_api->m_hook_pointer(
		*spec->m_site,
		spec->m_callback,
		spec->m_original
	);
	if (*spec->m_handle) {
		return true;
	}

	*spec->m_site = 0;
	if (spec->m_original) {
		*spec->m_original = NULL;
	}
	return false;
}

void dttr_import_hook_uninstall_spec(
	const DTTR_ComponentContext *ctx,
	const DTTR_ImportHookSpec *spec
) {
	if (!DTTR_IMPORT_HOOK_INSTALLED(spec)) {
		return;
	}

	ctx->m_game_api->m_unhook(*spec->m_handle);
	*spec->m_handle = NULL;
	if (spec->m_site) {
		*spec->m_site = 0;
	}
	if (spec->m_original) {
		*spec->m_original = NULL;
	}
}

static const DTTR_ImportHookSpec *s_find_spec(
	const DTTR_ImportHookSpec *specs,
	size_t spec_count,
	const char *import_name
) {
	for (size_t i = 0; i < spec_count; i++) {
		if (strcmp(specs[i].m_import_name, import_name) == 0) {
			return &specs[i];
		}
	}
	return NULL;
}

static const char *s_import_name(uint8_t *base, IMAGE_THUNK_DATA *name_thunk) {
	if (IMAGE_SNAP_BY_ORDINAL(name_thunk->u1.Ordinal)) {
		return NULL;
	}

	IMAGE_IMPORT_BY_NAME *import_name = (IMAGE_IMPORT_BY_NAME
											 *)(base + name_thunk->u1.AddressOfData);
	return (const char *)import_name->Name;
}

static void s_install_import_descriptor(
	const DTTR_ComponentContext *ctx,
	uint8_t *base,
	const char *module_name,
	IMAGE_IMPORT_DESCRIPTOR *desc,
	const DTTR_ImportHookSpec *specs,
	size_t spec_count
) {
	IMAGE_THUNK_DATA *name_thunk = (IMAGE_THUNK_DATA *)(base + desc->OriginalFirstThunk);
	IMAGE_THUNK_DATA *addr_thunk = (IMAGE_THUNK_DATA *)(base + desc->FirstThunk);
	for (; name_thunk->u1.AddressOfData; name_thunk++, addr_thunk++) {
		const char *import_name = s_import_name(base, name_thunk);
		if (!import_name) {
			DTTR_LOG_WARN("Unhandled ordinal import in %s", module_name);
			continue;
		}

		const DTTR_ImportHookSpec *spec = s_find_spec(specs, spec_count, import_name);
		if (!spec) {
			DTTR_LOG_ERROR("Unhandled import %s!%s", module_name, import_name);
			continue;
		}

		if (!spec->m_callback) {
			DTTR_LOG_DEBUG(
				"Skipping specialized import hook %s!%s",
				module_name,
				import_name
			);
			continue;
		}

		if (DTTR_IMPORT_HOOK_INSTALLED(spec)) {
			continue;
		}

		if (DTTR_IMPORT_INSTALL_SPEC(ctx, spec, &addr_thunk->u1.Function)) {
			DTTR_LOG_DEBUG(
				"Installed import hook %s!%s at 0x%08X",
				module_name,
				import_name,
				(unsigned)*spec->m_site
			);
		} else {
			DTTR_LOG_ERROR(
				"Failed to install import hook %s!%s",
				module_name,
				import_name
			);
		}
	}
}

void dttr_platform_hooks_install_module(
	const DTTR_ComponentContext *ctx,
	const char *module_name,
	const DTTR_ImportHookSpec *specs,
	size_t spec_count
) {
	uint8_t *base = (uint8_t *)ctx->m_game_module;
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
	IMAGE_DATA_DIRECTORY imports_dir = nt->OptionalHeader
										   .DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (!imports_dir.VirtualAddress) {
		return;
	}

	IMAGE_IMPORT_DESCRIPTOR *desc = (IMAGE_IMPORT_DESCRIPTOR
										 *)(base + imports_dir.VirtualAddress);
	for (; desc && desc->Name; desc++) {
		const char *desc_name = (const char *)(base + desc->Name);
		if (_stricmp(desc_name, module_name) != 0) {
			continue;
		}
		s_install_import_descriptor(ctx, base, module_name, desc, specs, spec_count);
		return;
	}
}

void dttr_platform_hooks_cleanup_module(
	const DTTR_ComponentContext *ctx,
	const DTTR_ImportHookSpec *specs,
	size_t spec_count
) {
	for (size_t i = spec_count; i > 0; i--) {
		const DTTR_ImportHookSpec *spec = &specs[i - 1];
		DTTR_IMPORT_UNINSTALL_SPEC(ctx, spec);
	}
}

void dttr_platform_hooks_init(const DTTR_ComponentContext *ctx) {
	g_dttr_import_hook_trace_enabled = dttr_log_is_enabled(LOG_TRACE) ? 1 : 0;

	dttr_platform_hooks_ddraw_init(ctx);
	dttr_platform_hooks_dinput_init(ctx);
	dttr_platform_hooks_gdi32_init(ctx);
	dttr_platform_hooks_kernel32_init(ctx);
	dttr_platform_hooks_user32_init(ctx);
	dttr_platform_hooks_winmm_init(ctx);
	dttr_platform_hooks_mss32_init(ctx);
	dttr_platform_hooks_winplay_init(ctx);
	dttr_platform_hooks_winstr_init(ctx);
}

void dttr_platform_hooks_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_winstr_cleanup(ctx);
	dttr_platform_hooks_winplay_cleanup(ctx);
	dttr_platform_hooks_mss32_cleanup(ctx);
	dttr_platform_hooks_winmm_cleanup(ctx);
	dttr_platform_hooks_user32_cleanup(ctx);
	dttr_platform_hooks_kernel32_cleanup(ctx);
	dttr_platform_hooks_gdi32_cleanup(ctx);
	dttr_platform_hooks_dinput_cleanup(ctx);
	dttr_platform_hooks_ddraw_cleanup(ctx);
}
