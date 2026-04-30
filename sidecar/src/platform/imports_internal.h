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

#define DTTR_IMPORT_TRACE_DECL_ENTRY(prefix, import_ident, import_name, import_log_name) \
	DTTR_IMPORT_HOOK_DECL(prefix, import_ident, import_log_name)
#define DTTR_IMPORT_WARN_DECL_ENTRY(prefix, import_ident, import_name, import_log_name)  \
	DTTR_IMPORT_HOOK_WARN_DECL(prefix, import_ident, import_log_name)
#define DTTR_IMPORT_SKIP_DECL_ENTRY(prefix, import_ident, import_name, import_log_name)

#define DTTR_IMPORT_TRACE_SPEC_ENTRY(prefix, import_ident, import_name, import_log_name) \
	DTTR_IMPORT_HOOK_SPEC(prefix, import_ident, import_name),
#define DTTR_IMPORT_WARN_SPEC_ENTRY(prefix, import_ident, import_name, import_log_name)  \
	DTTR_IMPORT_HOOK_SPEC(prefix, import_ident, import_name),
#define DTTR_IMPORT_SKIP_SPEC_ENTRY(prefix, import_ident, import_name, import_log_name)  \
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC(import_name),

#define DTTR_IMPORT_ENTRY_DECL(kind, prefix, import_ident, import_name, import_log_name) \
	DTTR_IMPORT_##kind##_DECL_ENTRY(prefix, import_ident, import_name, import_log_name)
#define DTTR_IMPORT_ENTRY_SPEC(kind, prefix, import_ident, import_name, import_log_name) \
	DTTR_IMPORT_##kind##_SPEC_ENTRY(prefix, import_ident, import_name, import_log_name)

#define DTTR_IMPORT_ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#endif /* DTTR_IMPORTS_INTERNAL_H */
