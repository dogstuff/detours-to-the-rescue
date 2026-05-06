#include "components_private.h"
#include "dttr_sidecar.h"
#include "game_api_private.h"
#include "hook_registry_private.h"

#include <dttr_config.h>
#include <dttr_log.h>
#include <dttr_path.h>

#include <kvec.h>
#include <sds.h>

#include <string.h>

#define S_COMPONENT_SHADOW_PREFIX "_dttr_hot_"
#define S_COMPONENT_MAX_SHADOW_ATTEMPTS 32
#define S_COMPONENT_RELOAD_STABLE_MS 500u
#define S_COMPONENT_RELOAD_POLL_MS 1000u

typedef kvec_t(S_LoadedComponent) S_ComponentVec;

static S_ComponentVec s_loaded_components;
static char s_components_dir[MAX_PATH];
static DWORD s_last_reload_scan_ms = 0;
static unsigned long s_shadow_counter = 0;
static uintptr_t s_hook_owner_counter = 0;

static DTTR_ComponentContext s_component_context(const DTTR_ComponentContext *base_ctx) {
	return (DTTR_ComponentContext){
		.m_api_version = base_ctx->m_api_version,
		.m_game_module = base_ctx->m_game_module,
		.m_sidecar_module = base_ctx->m_sidecar_module,
		.m_window = dttr_graphics_get_window(),
		.m_loader_dir = g_dttr_loader_dir,
		.m_exe_hash = g_dttr_exe_hash,
		.m_config = base_ctx->m_config,
		.m_api = base_ctx->m_api,
		.m_game_api = base_ctx->m_game_api,
	};
}

static bool s_file_id_equal(const S_ComponentFileId *lhs, const S_ComponentFileId *rhs) {
	return CompareFileTime(&lhs->m_write_time, &rhs->m_write_time) == 0
		   && lhs->m_size_high == rhs->m_size_high && lhs->m_size_low == rhs->m_size_low;
}

static S_ComponentFileId s_component_file_id(const WIN32_FIND_DATAA *find_data) {
	return (S_ComponentFileId){
		.m_write_time = find_data->ftLastWriteTime,
		.m_size_high = find_data->nFileSizeHigh,
		.m_size_low = find_data->nFileSizeLow,
	};
}

static bool s_make_component_path(sds *out, const char *filename) {
	*out = sdsnew(s_components_dir);
	return *out && dttr_path_append_segment(out, filename, '\\');
}

static void s_log_component_deleted(const S_LoadedComponent *component) {
	DTTR_LOG_INFO("Component deleted, unloading: %s", component->m_filename);
}

static bool s_is_shadow_component(const char *filename) {
	return strncmp(filename, S_COMPONENT_SHADOW_PREFIX, strlen(S_COMPONENT_SHADOW_PREFIX))
		   == 0;
}

static void s_log_component_info(const char *filename, DTTR_ComponentInfoFn info_fn) {
	if (!info_fn) {
		return;
	}

	const DTTR_ComponentInfo *info = info_fn();
	if (!info) {
		return;
	}

	DTTR_LOG_INFO(
		"Component: %s v%s by %s (%s)",
		info->m_name ? info->m_name : "unknown",
		info->m_version ? info->m_version : "?",
		info->m_author ? info->m_author : "unknown",
		filename
	);
}

#define S_COMPONENT_WITH_OWNER(component, call)                                          \
	do {                                                                                 \
		void *previous_owner = dttr_hook_set_owner((component)->m_hook_owner);           \
		call;                                                                            \
		dttr_hook_set_owner(previous_owner);                                             \
	} while (0)

#define S_COMPONENT_DISPATCH0(field)                                                     \
	do {                                                                                 \
		for (size_t i = 0; i < kv_size(s_loaded_components); i++) {                      \
			S_LoadedComponent *component = &kv_A(s_loaded_components, i);                \
			if (component->field) {                                                      \
				S_COMPONENT_WITH_OWNER(component, component->field());                   \
			}                                                                            \
		}                                                                                \
	} while (0)

#define S_COMPONENT_DISPATCH1(field, arg)                                                \
	do {                                                                                 \
		for (size_t i = 0; i < kv_size(s_loaded_components); i++) {                      \
			S_LoadedComponent *component = &kv_A(s_loaded_components, i);                \
			if (component->field) {                                                      \
				S_COMPONENT_WITH_OWNER(component, component->field(arg));                \
			}                                                                            \
		}                                                                                \
	} while (0)

#define S_COMPONENT_DISPATCH2(field, arg0, arg1)                                         \
	do {                                                                                 \
		for (size_t i = 0; i < kv_size(s_loaded_components); i++) {                      \
			S_LoadedComponent *component = &kv_A(s_loaded_components, i);                \
			if (component->field) {                                                      \
				S_COMPONENT_WITH_OWNER(component, component->field(arg0, arg1));         \
			}                                                                            \
		}                                                                                \
	} while (0)

#define S_COMPONENT_OPTIONAL_EXPORTS(X)                                                  \
	X(m_tick, DTTR_ComponentTickFn, "dttr_component_tick")                               \
	X(m_event, DTTR_ComponentEventFn, "dttr_component_event")                            \
	X(m_info, DTTR_ComponentInfoFn, "dttr_component_info")                               \
	X(m_late_init, DTTR_ComponentLateInitFn, "dttr_component_late_init")                 \
	X(m_before_unload, DTTR_ComponentBeforeUnloadFn, "dttr_component_before_unload")     \
	X(m_frame_begin, DTTR_ComponentFrameBeginFn, "dttr_component_frame_begin")           \
	X(m_before_game_frame,                                                               \
	  DTTR_ComponentBeforeGameFrameFn,                                                   \
	  "dttr_component_before_game_frame")                                                \
	X(m_after_game_frame,                                                                \
	  DTTR_ComponentAfterGameFrameFn,                                                    \
	  "dttr_component_after_game_frame")                                                 \
	X(m_before_present, DTTR_ComponentBeforePresentFn, "dttr_component_before_present")  \
	X(m_after_present, DTTR_ComponentAfterPresentFn, "dttr_component_after_present")     \
	X(m_frame_end, DTTR_ComponentFrameEndFn, "dttr_component_frame_end")                 \
	X(m_imgui_begin, DTTR_ComponentImguiBeginFn, "dttr_component_imgui_begin")           \
	X(m_imgui_end, DTTR_ComponentImguiEndFn, "dttr_component_imgui_end")                 \
	X(m_overlay_visible_changed,                                                         \
	  DTTR_ComponentOverlayVisibleChangedFn,                                             \
	  "dttr_component_overlay_visible_changed")                                          \
	X(m_window_created, DTTR_ComponentWindowCreatedFn, "dttr_component_window_created")  \
	X(m_window_resized, DTTR_ComponentWindowResizedFn, "dttr_component_window_resized")  \
	X(m_window_destroying,                                                               \
	  DTTR_ComponentWindowDestroyingFn,                                                  \
	  "dttr_component_window_destroying")                                                \
	X(m_graphics_device_created,                                                         \
	  DTTR_ComponentGraphicsDeviceCreatedFn,                                             \
	  "dttr_component_graphics_device_created")                                          \
	X(m_graphics_device_lost,                                                            \
	  DTTR_ComponentGraphicsDeviceLostFn,                                                \
	  "dttr_component_graphics_device_lost")                                             \
	X(m_graphics_device_restored,                                                        \
	  DTTR_ComponentGraphicsDeviceRestoredFn,                                            \
	  "dttr_component_graphics_device_restored")                                         \
	X(m_graphics_device_destroying,                                                      \
	  DTTR_ComponentGraphicsDeviceDestroyingFn,                                          \
	  "dttr_component_graphics_device_destroying")                                       \
	X(m_before_event, DTTR_ComponentBeforeEventFn, "dttr_component_before_event")        \
	X(m_after_event, DTTR_ComponentAfterEventFn, "dttr_component_after_event")           \
	X(m_input_mode_changed,                                                              \
	  DTTR_ComponentInputModeChangedFn,                                                  \
	  "dttr_component_input_mode_changed")                                               \
	X(m_render_game, DTTR_ComponentRenderGameFn, "dttr_component_render_game")           \
	X(m_render, DTTR_ComponentRenderFn, "dttr_component_render")                         \
	X(m_should_advance_game_frame,                                                       \
	  DTTR_ComponentShouldAdvanceGameFrameFn,                                            \
	  "dttr_component_should_advance_game_frame")                                        \
	X(m_game_frame_advanced,                                                             \
	  DTTR_ComponentGameFrameAdvancedFn,                                                 \
	  "dttr_component_game_frame_advanced")                                              \
	X(m_game_frame_blocked,                                                              \
	  DTTR_ComponentGameFrameBlockedFn,                                                  \
	  "dttr_component_game_frame_blocked")

static void s_delete_shadow_copy(S_LoadedComponent *component) {
	if (!component->m_shadow_path[0]) {
		return;
	}

	DeleteFileA(component->m_shadow_path);
	component->m_shadow_path[0] = '\0';
}

static int s_find_component(const char *filename) {
	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		if (strcmp(kv_A(s_loaded_components, i).m_filename, filename) == 0) {
			return (int)i;
		}
	}

	return -1;
}

static void s_unload_component(S_LoadedComponent *component) {
	if (!component->m_handle) {
		return;
	}

	if (component->m_initialized) {
		DTTR_LOG_INFO("Cleaning up component: %s", component->m_filename);
		if (component->m_before_unload) {
			S_COMPONENT_WITH_OWNER(component, component->m_before_unload());
		}
		dttr_hook_detach_owner(component->m_hook_owner);
		if (component->m_cleanup) {
			S_COMPONENT_WITH_OWNER(component, component->m_cleanup());
		}

		component->m_initialized = false;
	}

	dttr_hook_detach_owner(component->m_hook_owner);
	FreeLibrary(component->m_handle);
	component->m_handle = NULL;
	s_delete_shadow_copy(component);
}

static void s_remove_component_at(int index) {
	if (index < 0 || (size_t)index >= kv_size(s_loaded_components)) {
		return;
	}

	s_unload_component(&kv_A(s_loaded_components, index));
	const size_t last = kv_size(s_loaded_components) - 1;
	if ((size_t)index < last) {
		memmove(
			&kv_A(s_loaded_components, index),
			&kv_A(s_loaded_components, index + 1),
			(last - (size_t)index) * sizeof(kv_A(s_loaded_components, 0))
		);
	}

	s_loaded_components.n--;
}

static bool s_make_shadow_path(S_LoadedComponent *component) {
	const DWORD process_id = GetCurrentProcessId();

	for (int attempt = 0; attempt < S_COMPONENT_MAX_SHADOW_ATTEMPTS; attempt++) {
		sds shadow_name = sdscatprintf(
			sdsempty(),
			S_COMPONENT_SHADOW_PREFIX "%lu_%lu_%s",
			(unsigned long)process_id,
			++s_shadow_counter,
			component->m_filename
		);
		if (!shadow_name) {
			return false;
		}

		sds shadow_path = NULL;
		const bool made_shadow_path = s_make_component_path(&shadow_path, shadow_name);
		sdsfree(shadow_name);
		if (!made_shadow_path) {
			sdsfree(shadow_path);
			return false;
		}

		const bool copied_shadow_path = dttr_path_copy_sds(
			component->m_shadow_path,
			sizeof(component->m_shadow_path),
			shadow_path
		);
		sdsfree(shadow_path);
		if (!copied_shadow_path) {
			return false;
		}

		if (CopyFileA(component->m_source_path, component->m_shadow_path, TRUE)) {
			return true;
		}

		if (GetLastError() != ERROR_FILE_EXISTS) {
			return false;
		}
	}

	return false;
}

static void s_load_optional_exports(S_LoadedComponent *component) {
#define S_LOAD_OPTIONAL_EXPORT(field, fn_type, symbol)                                   \
	component->field = (fn_type)GetProcAddress(component->m_handle, symbol);

	S_COMPONENT_OPTIONAL_EXPORTS(S_LOAD_OPTIONAL_EXPORT)

#undef S_LOAD_OPTIONAL_EXPORT
}

static bool s_prepare_component(
	const char *filename,
	const char *source_path,
	const S_ComponentFileId *source_file,
	S_LoadedComponent *out
) {
	memset(out, 0, sizeof(*out));
	out->m_hook_owner = (void *)(++s_hook_owner_counter);
	dttr_path_copy_string(out->m_filename, sizeof(out->m_filename), filename);
	dttr_path_copy_string(out->m_source_path, sizeof(out->m_source_path), source_path);
	out->m_source_file = *source_file;

	const char *load_path = out->m_source_path;
	if (g_dttr_config.m_hot_reload) {
		if (!s_make_shadow_path(out)) {
			DTTR_LOG_WARN(
				"Failed to copy component DLL for hot reload: %s (error %lu)",
				filename,
				GetLastError()
			);
			return false;
		}

		load_path = out->m_shadow_path;
	}

	out->m_handle = LoadLibraryA(load_path);
	if (!out->m_handle) {
		DTTR_LOG_WARN(
			"Failed to load component DLL: %s (error %lu)",
			filename,
			GetLastError()
		);
		s_delete_shadow_copy(out);
		return false;
	}

	out->m_init = (DTTR_ComponentInitFn)
		GetProcAddress(out->m_handle, "dttr_component_init");
	out->m_cleanup = (DTTR_ComponentCleanupFn)
		GetProcAddress(out->m_handle, "dttr_component_cleanup");

	if (!out->m_init || !out->m_cleanup) {
		DTTR_LOG_WARN(
			"Component %s missing required exports "
			"(dttr_component_init/dttr_component_cleanup) - skipping",
			filename
		);
		s_unload_component(out);
		return false;
	}

	s_load_optional_exports(out);
	return true;
}

static bool s_init_component(S_LoadedComponent *component) {
	s_log_component_info(component->m_filename, component->m_info);

	const DTTR_ComponentContext *base_ctx = dttr_game_api_get_ctx();
	const DTTR_ComponentContext ctx = s_component_context(base_ctx);

	void *previous_owner = dttr_hook_set_owner(component->m_hook_owner);
	const bool initialized = component->m_init(&ctx);
	dttr_hook_set_owner(previous_owner);

	if (!initialized) {
		DTTR_LOG_WARN("Component %s init failed - skipping", component->m_filename);
		s_unload_component(component);
		return false;
	}

	component->m_initialized = true;
	component->m_reload_pending = false;
	return true;
}

static bool s_should_reload_now(
	S_LoadedComponent *component,
	const S_ComponentFileId *source_file,
	DWORD now_ms
) {
	if (s_file_id_equal(&component->m_source_file, source_file)) {
		component->m_reload_pending = false;
		return false;
	}

	if (!component->m_reload_pending
		|| !s_file_id_equal(&component->m_pending_file, source_file)) {
		component->m_pending_file = *source_file;
		component->m_pending_since_ms = now_ms;
		component->m_reload_pending = true;
		return false;
	}

	return now_ms - component->m_pending_since_ms >= S_COMPONENT_RELOAD_STABLE_MS;
}

static void s_load_component(
	const char *filename,
	const char *source_path,
	const S_ComponentFileId *source_file
) {
	if (kv_size(s_loaded_components) >= S_COMPONENTS_MAX) {
		DTTR_LOG_WARN(
			"Maximum component count (%d) reached - skipping %s",
			S_COMPONENTS_MAX,
			filename
		);
		return;
	}

	S_LoadedComponent component;
	if (!s_prepare_component(filename, source_path, source_file, &component)) {
		return;
	}

	if (!s_init_component(&component)) {
		return;
	}

	kv_push(S_LoadedComponent, s_loaded_components, component);
	DTTR_LOG_INFO("Loaded component: %s", filename);
}

static void s_reload_component(
	int index,
	const char *source_path,
	const S_ComponentFileId *source_file
) {
	S_LoadedComponent *old_component = &kv_A(s_loaded_components, index);

	S_LoadedComponent new_component;
	if (!s_prepare_component(
			old_component->m_filename,
			source_path,
			source_file,
			&new_component
		)) {
		return;
	}

	DTTR_LOG_INFO("Reloading component: %s", old_component->m_filename);
	s_unload_component(old_component);

	if (!s_init_component(&new_component)) {
		s_remove_component_at(index);
		return;
	}

	kv_A(s_loaded_components, index) = new_component;
}

static void s_remove_missing_components(const bool *seen, bool initial_scan) {
	if (initial_scan) {
		return;
	}

	for (int i = (int)kv_size(s_loaded_components) - 1; i >= 0; i--) {
		if (seen[i]) {
			continue;
		}

		s_log_component_deleted(&kv_A(s_loaded_components, i));
		s_remove_component_at(i);
	}
}

static void s_remove_all_components(bool log_deleted) {
	while (kv_size(s_loaded_components) > 0) {
		const int i = (int)kv_size(s_loaded_components) - 1;
		if (log_deleted) {
			s_log_component_deleted(&kv_A(s_loaded_components, i));
		}

		s_remove_component_at(i);
	}
}

static void s_scan_component_file(
	const WIN32_FIND_DATAA *find_data,
	DWORD now_ms,
	bool initial_scan,
	bool *seen
) {
	if (find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		return;
	}

	if (s_is_shadow_component(find_data->cFileName)) {
		return;
	}

	if (dttr_config_is_component_disabled(&g_dttr_config, find_data->cFileName)) {
		DTTR_LOG_INFO("Component disabled, skipping: %s", find_data->cFileName);
		return;
	}

	sds source_path = NULL;
	if (!s_make_component_path(&source_path, find_data->cFileName)) {
		sdsfree(source_path);
		return;
	}

	const S_ComponentFileId source_file = s_component_file_id(find_data);
	const int index = s_find_component(find_data->cFileName);
	if (index < 0) {
		s_load_component(find_data->cFileName, source_path, &source_file);
		const int new_index = s_find_component(find_data->cFileName);
		if (new_index >= 0) {
			seen[new_index] = true;
		}

		sdsfree(source_path);
		return;
	}

	seen[index] = true;
	if (!initial_scan
		&& s_should_reload_now(&kv_A(s_loaded_components, index), &source_file, now_ms)) {
		s_reload_component(index, source_path, &source_file);
	}

	sdsfree(source_path);
}

static void s_scan_components(bool initial_scan) {
	bool seen[S_COMPONENTS_MAX] = {0};

	sds search_pattern = NULL;
	if (!s_make_component_path(&search_pattern, "*.dll")) {
		sdsfree(search_pattern);
		return;
	}

	WIN32_FIND_DATAA find_data;
	HANDLE find_handle = FindFirstFileA(search_pattern, &find_data);
	sdsfree(search_pattern);

	if (find_handle == INVALID_HANDLE_VALUE) {
		if (initial_scan) {
			DTTR_LOG_INFO("Loaded 0 component(s)");
		}

		s_remove_all_components(!initial_scan);
		return;
	}

	const DWORD now_ms = GetTickCount();

	do {
		s_scan_component_file(&find_data, now_ms, initial_scan, seen);
	} while (FindNextFileA(find_handle, &find_data));

	FindClose(find_handle);
	s_remove_missing_components(seen, initial_scan);
}

static bool s_resolve_components_dir(void) {
	sds components_dir = sdsnew(g_dttr_loader_dir);
	if (!components_dir
		|| !dttr_path_append_segment(&components_dir, "components", '\\')) {
		sdsfree(components_dir);
		return false;
	}

	const DWORD attrs = GetFileAttributesA(components_dir);
	if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
		DTTR_LOG_INFO("No components directory found at %s - skipping", components_dir);
		sdsfree(components_dir);
		return false;
	}

	const bool copied = dttr_path_copy_sds(
		s_components_dir,
		sizeof(s_components_dir),
		components_dir
	);
	sdsfree(components_dir);
	return copied;
}

static void s_maybe_hot_reload_components(void) {
	if (!g_dttr_config.m_hot_reload) {
		return;
	}

	const DWORD now_ms = GetTickCount();
	if (now_ms - s_last_reload_scan_ms < S_COMPONENT_RELOAD_POLL_MS) {
		return;
	}

	s_last_reload_scan_ms = now_ms;
	s_scan_components(false);
}

void dttr_components_init(void) {
	DTTR_LOG_INFO("Loading components...");

	if (!s_resolve_components_dir()) {
		return;
	}

	s_scan_components(true);
	s_last_reload_scan_ms = GetTickCount();

	DTTR_LOG_INFO("Loaded %d component(s)", (int)kv_size(s_loaded_components));
}

void dttr_components_tick(void) {
	s_maybe_hot_reload_components();

	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		S_LoadedComponent *component = &kv_A(s_loaded_components, i);
		if (!component->m_tick) {
			continue;
		}

		S_COMPONENT_WITH_OWNER(component, component->m_tick());
	}
}

void dttr_components_late_init(void) { S_COMPONENT_DISPATCH0(m_late_init); }

void dttr_components_before_unload(void) { S_COMPONENT_DISPATCH0(m_before_unload); }

void dttr_components_frame_begin(const DTTR_FrameContext *ctx) {
	S_COMPONENT_DISPATCH1(m_frame_begin, ctx);
}

void dttr_components_before_game_frame(const DTTR_FrameContext *ctx) {
	S_COMPONENT_DISPATCH1(m_before_game_frame, ctx);
}

void dttr_components_after_game_frame(const DTTR_FrameContext *ctx) {
	S_COMPONENT_DISPATCH1(m_after_game_frame, ctx);
}

void dttr_components_before_present(const DTTR_PresentContext *ctx) {
	S_COMPONENT_DISPATCH1(m_before_present, ctx);
}

void dttr_components_after_present(const DTTR_PresentContext *ctx) {
	S_COMPONENT_DISPATCH1(m_after_present, ctx);
}

void dttr_components_frame_end(const DTTR_FrameContext *ctx) {
	S_COMPONENT_DISPATCH1(m_frame_end, ctx);
}

void dttr_components_imgui_begin(const DTTR_RenderContext *ctx) {
	S_COMPONENT_DISPATCH1(m_imgui_begin, ctx);
}

void dttr_components_imgui_end(const DTTR_RenderContext *ctx) {
	S_COMPONENT_DISPATCH1(m_imgui_end, ctx);
}

void dttr_components_overlay_visible_changed(bool visible) {
	S_COMPONENT_DISPATCH1(m_overlay_visible_changed, visible);
}

void dttr_components_window_created(const DTTR_WindowContext *ctx) {
	S_COMPONENT_DISPATCH1(m_window_created, ctx);
}

void dttr_components_window_resized(const DTTR_WindowContext *ctx) {
	S_COMPONENT_DISPATCH1(m_window_resized, ctx);
}

void dttr_components_window_destroying(const DTTR_WindowContext *ctx) {
	S_COMPONENT_DISPATCH1(m_window_destroying, ctx);
}

void dttr_components_graphics_device_created(const DTTR_GraphicsContext *ctx) {
	S_COMPONENT_DISPATCH1(m_graphics_device_created, ctx);
}

void dttr_components_graphics_device_lost(const DTTR_GraphicsContext *ctx) {
	S_COMPONENT_DISPATCH1(m_graphics_device_lost, ctx);
}

void dttr_components_graphics_device_restored(const DTTR_GraphicsContext *ctx) {
	S_COMPONENT_DISPATCH1(m_graphics_device_restored, ctx);
}

void dttr_components_graphics_device_destroying(const DTTR_GraphicsContext *ctx) {
	S_COMPONENT_DISPATCH1(m_graphics_device_destroying, ctx);
}

bool dttr_components_before_event(const SDL_Event *event) {
	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		S_LoadedComponent *component = &kv_A(s_loaded_components, i);
		if (!component->m_before_event) {
			continue;
		}
		bool consumed = false;
		S_COMPONENT_WITH_OWNER(component, consumed = component->m_before_event(event));
		if (consumed) {
			return true;
		}
	}
	return false;
}

void dttr_components_after_event(const SDL_Event *event, bool consumed) {
	S_COMPONENT_DISPATCH2(m_after_event, event, consumed);
}

void dttr_components_input_mode_changed(const DTTR_InputContext *ctx) {
	S_COMPONENT_DISPATCH1(m_input_mode_changed, ctx);
}

bool dttr_components_should_advance_game_frame(void) {
	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		S_LoadedComponent *component = &kv_A(s_loaded_components, i);
		if (!component->m_should_advance_game_frame) {
			continue;
		}

		bool should_advance = true;
		S_COMPONENT_WITH_OWNER(
			component,
			should_advance = component->m_should_advance_game_frame()
		);
		if (!should_advance) {
			return false;
		}
	}

	return true;
}

void dttr_components_game_frame_advanced(void) {
	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		S_LoadedComponent *component = &kv_A(s_loaded_components, i);
		if (!component->m_game_frame_advanced) {
			continue;
		}

		S_COMPONENT_WITH_OWNER(component, component->m_game_frame_advanced());
	}
}

void dttr_components_game_frame_blocked(void) {
	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		S_LoadedComponent *component = &kv_A(s_loaded_components, i);
		if (!component->m_game_frame_blocked) {
			continue;
		}

		S_COMPONENT_WITH_OWNER(component, component->m_game_frame_blocked());
	}
}

bool dttr_components_has_render_game(void) {
	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		if (kv_A(s_loaded_components, i).m_render_game) {
			return true;
		}
	}

	return false;
}

void dttr_components_render_game(const DTTR_RenderGameContext *ctx) {
	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		S_LoadedComponent *component = &kv_A(s_loaded_components, i);
		if (!component->m_render_game) {
			continue;
		}

		S_COMPONENT_WITH_OWNER(component, component->m_render_game(ctx));
	}
}

void dttr_components_render(const DTTR_RenderContext *ctx) {
	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		S_LoadedComponent *component = &kv_A(s_loaded_components, i);
		if (!component->m_render) {
			continue;
		}

		S_COMPONENT_WITH_OWNER(component, component->m_render(ctx));
	}
}

bool dttr_components_handle_event(const SDL_Event *event) {
	for (size_t i = 0; i < kv_size(s_loaded_components); i++) {
		S_LoadedComponent *component = &kv_A(s_loaded_components, i);
		if (!component->m_event) {
			continue;
		}

		bool consumed = false;
		S_COMPONENT_WITH_OWNER(component, consumed = component->m_event(event));
		if (consumed) {
			return true;
		}
	}

	return false;
}

void dttr_components_cleanup(void) {
	s_remove_all_components(false);
	kv_destroy(s_loaded_components);
	kv_init(s_loaded_components);
	s_components_dir[0] = '\0';
	s_last_reload_scan_ms = 0;
}
