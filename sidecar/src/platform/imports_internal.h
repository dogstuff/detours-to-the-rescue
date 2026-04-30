#ifndef DTTR_IMPORTS_INTERNAL_H
#define DTTR_IMPORTS_INTERNAL_H

#include <dttr_components.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

typedef struct DTTR_ImportHookSpec {
	const char *m_import_name;
	void *m_callback;
	void **m_original;
	uintptr_t *m_site;
	DTTR_Hook **m_handle;
} DTTR_ImportHookSpec;

void dttr_platform_hooks_init(const DTTR_ComponentContext *ctx);
void dttr_platform_hooks_cleanup(const DTTR_ComponentContext *ctx);

void dttr_import_hook_trace(const char *name);
void dttr_import_hook_warn(const char *name);
bool dttr_import_hook_install_spec(
	const DTTR_ComponentContext *ctx,
	const DTTR_ImportHookSpec *spec,
	uintptr_t site
);
void dttr_import_hook_uninstall_spec(
	const DTTR_ComponentContext *ctx,
	const DTTR_ImportHookSpec *spec
);
void dttr_platform_hooks_install_module(
	const DTTR_ComponentContext *ctx,
	const char *module_name,
	const DTTR_ImportHookSpec *specs,
	size_t spec_count
);
void dttr_platform_hooks_cleanup_module(
	const DTTR_ComponentContext *ctx,
	const DTTR_ImportHookSpec *specs,
	size_t spec_count
);

#endif /* DTTR_IMPORTS_INTERNAL_H */
