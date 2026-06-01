#include "dttr_sidecar.h"
#include "mods_private.h"
#include "sidecar_private.h"
#include <dttr_runtime.h>

#include <dttr_config.h>
#include <dttr_errors.h>
#include <dttr_log.h>
#include <dttr_path.h>

#include <kvec.h>
#include <sds.h>

#include <stdlib.h>
#include <string.h>

#define MOD_MAX_SHADOW_ATTEMPTS 32
#define MOD_RELOAD_STABLE_MS 500u
#define MOD_RELOAD_POLL_MS 1000u

typedef kvec_t(loaded_mod) mod_vec;

static mod_vec loaded_mods;
static char mods_dir[MAX_PATH];
static DWORD last_reload_scan_ms = 0;
static unsigned long shadow_counter = 0;
static uintptr_t hook_owner_counter = 0;

static DTTR_Mods_Context mod_context(const DTTR_Mods_Context *base_ctx) {
	return (DTTR_Mods_Context){
		.abi_version = base_ctx->abi_version,
		.runtime = base_ctx->runtime,
		.sidecar_module = base_ctx->sidecar_module,
		.window = DTTR_Graphics_GetWindow(),
		.loader_dir = dttr_loader_dir,
		.exe_hash = dttr_exe_hash,
		.config = base_ctx->config,
		.api = base_ctx->api,
		.struct_size = sizeof(DTTR_Mods_Context),
	};
}

static bool file_id_equal(const mod_file_id *lhs, const mod_file_id *rhs) {
	return CompareFileTime(&lhs->write_time, &rhs->write_time) == 0
		   && lhs->size_high == rhs->size_high && lhs->size_low == rhs->size_low;
}

static mod_file_id make_mod_file_id(const WIN32_FIND_DATAA *find_data) {
	return (mod_file_id){
		.write_time = find_data->ftLastWriteTime,
		.size_high = find_data->nFileSizeHigh,
		.size_low = find_data->nFileSizeLow,
	};
}

static bool make_mod_path(sds *out, const char *filename) {
	*out = sdsnew(mods_dir);
	return *out && DTTR_Path_AppendSegment(out, filename, '\\');
}

static void log_mod_deleted(const loaded_mod *mod) {
	DTTR_LOG_INFO("Mod deleted, unloading: %s", mod->filename);
}

// Recognizes temporary shadow-copy DLLs so hot reload never treats them as source mods.
static bool is_shadow_mod(const char *filename) {
	return strncmp(filename, DTTR_MODS_SHADOW_PREFIX, strlen(DTTR_MODS_SHADOW_PREFIX))
		   == 0;
}

static const DTTR_Mods_Info *get_mod_info(DTTR_Mods_InfoFn info_fn) {
	return info_fn ? info_fn() : NULL;
}

static void set_mod_display_name(loaded_mod *mod, const DTTR_Mods_Info *info) {
	const char *name = info && info->name && info->name[0] ? info->name : mod->filename;
	DTTR_Path_CopyString(mod->display_name, sizeof(mod->display_name), name);
}

static void log_mod_info(const char *filename, const DTTR_Mods_Info *info) {
	if (!info) {
		return;
	}

	DTTR_LOG_INFO(
		"Mod: %s v%s by %s (%s)",
		info->name ? info->name : "unknown",
		info->version ? info->version : "?",
		info->author ? info->author : "unknown",
		filename
	);
}

static void destroy_mod_context(loaded_mod *mod) {
	free(mod->context);
	mod->context = NULL;
}

// Builds the stable context pointer exposed from DTTR_Mod_Init through cleanup.
static bool refresh_mod_context(loaded_mod *mod, const DTTR_Mods_Context *base_ctx) {
	if (!mod->context) {
		mod->context = (DTTR_Mods_Context *)calloc(1, sizeof(*mod->context));
		if (!mod->context) {
			DTTR_LOG_WARN("Failed to allocate mod context for %s", mod->filename);
			return false;
		}
	}

	*mod->context = mod_context(base_ctx);
	return true;
}

#define MOD_WITH_OWNER(mod, call)                                                        \
	do {                                                                                 \
		void *previous_owner = DTTR_Core_HookSetOwner((mod)->hook_owner);                \
		call;                                                                            \
		DTTR_Core_HookSetOwner(previous_owner);                                          \
	} while (0)

#define MOD_DISPATCH(field, ...)                                                         \
	do {                                                                                 \
		for (size_t i = 0; i < kv_size(loaded_mods); i++) {                              \
			loaded_mod *mod = &kv_A(loaded_mods, i);                                     \
			if (mod->field) {                                                            \
				MOD_WITH_OWNER(mod, mod->field(__VA_ARGS__));                            \
			}                                                                            \
		}                                                                                \
	} while (0)

#define MOD_OPTIONAL_EXPORTS(X)                                                          \
	X(tick, DTTR_Mods_TickFn, "DTTR_Mod_Tick")                                           \
	X(event, DTTR_Mods_EventFn, "DTTR_Mod_Event")                                        \
	X(info, DTTR_Mods_InfoFn, "DTTR_Mod_Info")                                           \
	X(late_init, DTTR_Mods_LateInitFn, "DTTR_Mod_LateInit")                              \
	X(before_unload, DTTR_Mods_BeforeUnloadFn, "DTTR_Mod_BeforeUnload")                  \
	X(frame_begin, DTTR_Mods_FrameBeginFn, "DTTR_Mod_FrameBegin")                        \
	X(before_game_frame, DTTR_Mods_BeforeGameFrameFn, "DTTR_Mod_BeforeGameFrame")        \
	X(after_game_frame, DTTR_Mods_AfterGameFrameFn, "DTTR_Mod_AfterGameFrame")           \
	X(before_present, DTTR_Mods_BeforePresentFn, "DTTR_Mod_BeforePresent")               \
	X(after_present, DTTR_Mods_AfterPresentFn, "DTTR_Mod_AfterPresent")                  \
	X(frame_end, DTTR_Mods_FrameEndFn, "DTTR_Mod_FrameEnd")                              \
	X(imgui_begin, DTTR_Mods_ImGuiBeginFn, "DTTR_Mod_ImGuiBegin")                        \
	X(imgui_end, DTTR_Mods_ImGuiEndFn, "DTTR_Mod_ImGuiEnd")                              \
	X(overlay_visible_changed,                                                           \
	  DTTR_Mods_OverlayVisibleChangedFn,                                                 \
	  "DTTR_Mod_OverlayVisibleChanged")                                                  \
	X(window_created, DTTR_Mods_WindowCreatedFn, "DTTR_Mod_WindowCreated")               \
	X(window_resized, DTTR_Mods_WindowResizedFn, "DTTR_Mod_WindowResized")               \
	X(window_destroying, DTTR_Mods_WindowDestroyingFn, "DTTR_Mod_WindowDestroying")      \
	X(graphics_device_created,                                                           \
	  DTTR_Mods_GraphicsDeviceCreatedFn,                                                 \
	  "DTTR_Mod_GraphicsDeviceCreated")                                                  \
	X(graphics_device_lost,                                                              \
	  DTTR_Mods_GraphicsDeviceLostFn,                                                    \
	  "DTTR_Mod_GraphicsDeviceLost")                                                     \
	X(graphics_device_restored,                                                          \
	  DTTR_Mods_GraphicsDeviceRestoredFn,                                                \
	  "DTTR_Mod_GraphicsDeviceRestored")                                                 \
	X(graphics_device_destroying,                                                        \
	  DTTR_Mods_GraphicsDeviceDestroyingFn,                                              \
	  "DTTR_Mod_GraphicsDeviceDestroying")                                               \
	X(before_event, DTTR_Mods_BeforeEventFn, "DTTR_Mod_BeforeEvent")                     \
	X(after_event, DTTR_Mods_AfterEventFn, "DTTR_Mod_AfterEvent")                        \
	X(input_mode_changed, DTTR_Mods_InputModeChangedFn, "DTTR_Mod_InputModeChanged")     \
	X(render_game, DTTR_Mods_RenderGameFn, "DTTR_Mod_RenderGame")                        \
	X(render, DTTR_Mods_RenderFn, "DTTR_Mod_Render")                                     \
	X(should_advance_game_frame,                                                         \
	  DTTR_Mods_ShouldAdvanceGameFrameFn,                                                \
	  "DTTR_Mod_ShouldAdvanceGameFrame")                                                 \
	X(game_frame_advanced, DTTR_Mods_GameFrameAdvancedFn, "DTTR_Mod_GameFrameAdvanced")  \
	X(game_frame_blocked, DTTR_Mods_GameFrameBlockedFn, "DTTR_Mod_GameFrameBlocked")

static void delete_shadow_copy(loaded_mod *mod) {
	if (!mod->shadow_path[0]) {
		return;
	}

	DeleteFileA(mod->shadow_path);
	mod->shadow_path[0] = '\0';
}

static int find_mod(const char *filename) {
	for (size_t i = 0; i < kv_size(loaded_mods); i++) {
		if (strcmp(kv_A(loaded_mods, i).filename, filename) == 0) {
			return (int)i;
		}
	}

	return -1;
}

// Runs mod unload callbacks and removes hook ownership before freeing the DLL.
static void unload_mod(loaded_mod *mod) {
	if (!mod->handle) {
		destroy_mod_context(mod);
		return;
	}

	if (mod->initialized) {
		DTTR_LOG_INFO("Cleaning up mod: %s", mod->filename);
		if (mod->before_unload) {
			MOD_WITH_OWNER(mod, mod->before_unload());
		}

		if (mod->cleanup) {
			MOD_WITH_OWNER(mod, mod->cleanup());
		}

		mod->initialized = false;
	}

	if (!DTTR_Core_HookDetachOwnerChecked(mod->hook_owner)) {
		DTTR_FATAL(
			"Refusing to unload mod %s because one or more hooks could not be restored",
			mod->filename
		);
	}

	FreeLibrary(mod->handle);
	mod->handle = NULL;
	delete_shadow_copy(mod);
	destroy_mod_context(mod);
}

static void remove_mod_at(int index) {
	if (index < 0 || (size_t)index >= kv_size(loaded_mods)) {
		return;
	}

	unload_mod(&kv_A(loaded_mods, index));
	const size_t last = kv_size(loaded_mods) - 1;
	if ((size_t)index < last) {
		memmove(
			&kv_A(loaded_mods, index),
			&kv_A(loaded_mods, index + 1),
			(last - (size_t)index) * sizeof(kv_A(loaded_mods, 0))
		);
	}

	loaded_mods.n--;
}

// Builds a unique shadow-copy path so Windows can reload a changed mod DLL.
static bool make_shadow_path(loaded_mod *mod) {
	const DWORD process_id = GetCurrentProcessId();

	for (int attempt = 0; attempt < MOD_MAX_SHADOW_ATTEMPTS; attempt++) {
		sds shadow_name = sdscatprintf(
			sdsempty(),
			DTTR_MODS_SHADOW_PREFIX "%lu_%lu_%s",
			(unsigned long)process_id,
			++shadow_counter,
			mod->filename
		);
		if (!shadow_name) {
			return false;
		}

		sds shadow_path = NULL;
		const bool made_shadow_path = make_mod_path(&shadow_path, shadow_name);
		sdsfree(shadow_name);
		if (!made_shadow_path) {
			sdsfree(shadow_path);
			return false;
		}

		const bool copied_shadow_path = DTTR_Path_CopySds(
			mod->shadow_path,
			sizeof(mod->shadow_path),
			shadow_path
		);
		sdsfree(shadow_path);
		if (!copied_shadow_path) {
			return false;
		}

		if (CopyFileA(mod->source_path, mod->shadow_path, TRUE)) {
			return true;
		}

		if (GetLastError() != ERROR_FILE_EXISTS) {
			return false;
		}
	}

	return false;
}

static void load_optional_exports(loaded_mod *mod) {
#define LOAD_OPTIONAL_EXPORT(field, fn_type, symbol)                                     \
	mod->field = (fn_type)GetProcAddress(mod->handle, symbol);

	MOD_OPTIONAL_EXPORTS(LOAD_OPTIONAL_EXPORT)

#undef LOAD_OPTIONAL_EXPORT
}

static bool prepare_mod(
	const char *filename,
	const char *source_path,
	const mod_file_id *source_file,
	loaded_mod *out
) {
	memset(out, 0, sizeof(*out));
	out->hook_owner = (void *)(++hook_owner_counter);
	DTTR_Path_CopyString(out->filename, sizeof(out->filename), filename);
	DTTR_Path_CopyString(out->source_path, sizeof(out->source_path), source_path);
	out->source_file = *source_file;

	const char *load_path = out->source_path;
	if (dttr_config.hot_reload) {
		if (!make_shadow_path(out)) {
			DTTR_LOG_WARN(
				"Failed to copy mod DLL for hot reload: %s (error %lu)",
				filename,
				GetLastError()
			);
			return false;
		}

		load_path = out->shadow_path;
	}

	out->handle = LoadLibraryA(load_path);
	if (!out->handle) {
		DTTR_LOG_WARN("Failed to load mod DLL: %s (error %lu)", filename, GetLastError());
		delete_shadow_copy(out);
		return false;
	}

	out->init = (DTTR_Mods_InitFn)GetProcAddress(out->handle, "DTTR_Mod_Init");
	out->cleanup = (DTTR_Mods_CleanupFn)GetProcAddress(out->handle, "DTTR_Mod_Cleanup");

	if (!out->init || !out->cleanup) {
		DTTR_LOG_WARN(
			"Mod %s missing required exports "
			"(DTTR_Mod_Init/DTTR_Mod_Cleanup) - skipping",
			filename
		);
		unload_mod(out);
		return false;
	}

	load_optional_exports(out);
	return true;
}

// Calls mod initialization and records ownership for hooks installed by that DLL.
static bool init_mod(loaded_mod *mod) {
	const DTTR_Mods_Info *info = get_mod_info(mod->info);
	set_mod_display_name(mod, info);
	log_mod_info(mod->filename, info);

	const DTTR_Mods_Context *base_ctx = dttr_sidecar_context();
	if (!refresh_mod_context(mod, base_ctx)) {
		unload_mod(mod);
		return false;
	}

	void *previous_owner = DTTR_Core_HookSetOwner(mod->hook_owner);
	const bool initialized = mod->init(mod->context);
	DTTR_Core_HookSetOwner(previous_owner);

	if (!initialized) {
		DTTR_LOG_WARN("Mod %s init failed - skipping", mod->filename);
		unload_mod(mod);
		return false;
	}

	mod->initialized = true;
	mod->reload_pending = false;
	mod->loaded_at_ms = GetTickCount();
	return true;
}

static bool should_reload_now(
	loaded_mod *mod,
	const mod_file_id *source_file,
	DWORD now_ms
) {
	if (file_id_equal(&mod->source_file, source_file)) {
		mod->reload_pending = false;
		return false;
	}

	if (!mod->reload_pending || !file_id_equal(&mod->pending_file, source_file)) {
		mod->pending_file = *source_file;
		mod->pending_since_ms = now_ms;
		mod->reload_pending = true;
		return false;
	}

	return now_ms - mod->pending_since_ms >= MOD_RELOAD_STABLE_MS;
}

static void load_mod(
	const char *filename,
	const char *source_path,
	const mod_file_id *source_file
) {
	if (kv_size(loaded_mods) >= MODS_MAX) {
		DTTR_LOG_WARN("Maximum mod count (%d) reached - skipping %s", MODS_MAX, filename);
		return;
	}

	loaded_mod mod;
	if (!prepare_mod(filename, source_path, source_file, &mod)) {
		return;
	}

	if (!init_mod(&mod)) {
		return;
	}

	kv_push(loaded_mod, loaded_mods, mod);
	DTTR_LOG_INFO("Loaded mod: %s", filename);
}

static bool reload_mod(int index, const char *source_path, const mod_file_id *source_file) {
	loaded_mod *old_mod = &kv_A(loaded_mods, index);

	loaded_mod new_mod;
	if (!prepare_mod(old_mod->filename, source_path, source_file, &new_mod)) {
		return false;
	}

	DTTR_LOG_INFO("Reloading mod: %s", old_mod->filename);
	unload_mod(old_mod);

	if (!init_mod(&new_mod)) {
		remove_mod_at(index);
		return true;
	}

	kv_A(loaded_mods, index) = new_mod;
	return false;
}

static void remove_missing_mods(const bool *seen, bool initial_scan) {
	if (initial_scan) {
		return;
	}

	for (int i = (int)kv_size(loaded_mods) - 1; i >= 0; i--) {
		if (seen[i]) {
			continue;
		}

		log_mod_deleted(&kv_A(loaded_mods, i));
		remove_mod_at(i);
	}
}

static void remove_all_mods(bool log_deleted) {
	while (kv_size(loaded_mods) > 0) {
		const int i = (int)kv_size(loaded_mods) - 1;
		if (log_deleted) {
			log_mod_deleted(&kv_A(loaded_mods, i));
		}

		remove_mod_at(i);
	}
}

static bool scan_mod_file(
	const WIN32_FIND_DATAA *find_data,
	DWORD now_ms,
	bool initial_scan,
	bool *seen
) {
	if (find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		return false;
	}

	if (is_shadow_mod(find_data->cFileName)) {
		return false;
	}

	if (DTTR_Config_IsModDisabled(&dttr_config, find_data->cFileName)) {
		DTTR_LOG_INFO("Mod disabled, skipping: %s", find_data->cFileName);
		return false;
	}

	sds source_path = NULL;
	if (!make_mod_path(&source_path, find_data->cFileName)) {
		sdsfree(source_path);
		return false;
	}

	const mod_file_id source_file = make_mod_file_id(find_data);
	const int index = find_mod(find_data->cFileName);
	if (index < 0) {
		load_mod(find_data->cFileName, source_path, &source_file);
		const int new_index = find_mod(find_data->cFileName);
		if (new_index >= 0) {
			seen[new_index] = true;
		}

		sdsfree(source_path);
		return false;
	}

	seen[index] = true;
	bool restart_scan = false;
	if (!initial_scan
		&& should_reload_now(&kv_A(loaded_mods, index), &source_file, now_ms)) {
		restart_scan = reload_mod(index, source_path, &source_file);
	}

	sdsfree(source_path);
	return restart_scan;
}

static void scan_mods(bool initial_scan) {
	for (;;) {
		bool seen[MODS_MAX] = {0};

		sds search_pattern = NULL;
		if (!make_mod_path(&search_pattern, "*.dll")) {
			sdsfree(search_pattern);
			return;
		}

		WIN32_FIND_DATAA find_data;
		HANDLE find_handle = FindFirstFileA(search_pattern, &find_data);
		sdsfree(search_pattern);

		if (find_handle == INVALID_HANDLE_VALUE) {
			if (initial_scan) {
				DTTR_LOG_INFO("Loaded 0 mod(s)");
			}

			remove_all_mods(!initial_scan);
			return;
		}

		const DWORD now_ms = GetTickCount();
		bool restart_scan = false;

		do {
			if (scan_mod_file(&find_data, now_ms, initial_scan, seen)) {
				restart_scan = true;
				break;
			}
		} while (FindNextFileA(find_handle, &find_data));

		FindClose(find_handle);
		if (restart_scan) {
			continue;
		}

		remove_missing_mods(seen, initial_scan);
		return;
	}
}

// Resolves the mod directory relative to the loader so mods live beside the game.
static bool resolve_mods_dir() {
	sds resolved_mods_dir = sdsnew(dttr_loader_dir);
	if (!resolved_mods_dir
		|| !DTTR_Path_AppendSegment(&resolved_mods_dir, "mods", '\\')) {
		sdsfree(resolved_mods_dir);
		return false;
	}

	const DWORD attrs = GetFileAttributesA(resolved_mods_dir);
	if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
		DTTR_LOG_INFO("No mods directory found at %s - skipping", resolved_mods_dir);
		sdsfree(resolved_mods_dir);
		return false;
	}

	const bool copied = DTTR_Path_CopySds(mods_dir, sizeof(mods_dir), resolved_mods_dir);
	sdsfree(resolved_mods_dir);
	return copied;
}

static void attempt_hot_reload_mods() {
	if (!dttr_config.hot_reload) {
		return;
	}

	const DWORD now_ms = GetTickCount();
	if (now_ms - last_reload_scan_ms < MOD_RELOAD_POLL_MS) {
		return;
	}

	last_reload_scan_ms = now_ms;
	scan_mods(false);
}

void dttr_mods_init() {
	DTTR_LOG_INFO("Loading mods...");

	if (!resolve_mods_dir()) {
		return;
	}

	scan_mods(true);
	last_reload_scan_ms = GetTickCount();

	DTTR_LOG_INFO("Loaded %d mod(s)", (int)kv_size(loaded_mods));
}

void dttr_mods_tick() {
	attempt_hot_reload_mods();

	for (size_t i = 0; i < kv_size(loaded_mods); i++) {
		loaded_mod *mod = &kv_A(loaded_mods, i);
		if (!mod->tick) {
			continue;
		}

		MOD_WITH_OWNER(mod, mod->tick());
	}
}

void dttr_mods_late_init() { MOD_DISPATCH(late_init); }

void dttr_mods_before_unload() { MOD_DISPATCH(before_unload); }

void dttr_mods_frame_begin(const DTTR_Mods_FrameContext *ctx) {
	MOD_DISPATCH(frame_begin, ctx);
}

void dttr_mods_before_game_frame(const DTTR_Mods_FrameContext *ctx) {
	MOD_DISPATCH(before_game_frame, ctx);
}

void dttr_mods_after_game_frame(const DTTR_Mods_FrameContext *ctx) {
	MOD_DISPATCH(after_game_frame, ctx);
}

void dttr_mods_before_present(const DTTR_Mods_PresentContext *ctx) {
	MOD_DISPATCH(before_present, ctx);
}

void dttr_mods_after_present(const DTTR_Mods_PresentContext *ctx) {
	MOD_DISPATCH(after_present, ctx);
}

void dttr_mods_frame_end(const DTTR_Mods_FrameContext *ctx) {
	MOD_DISPATCH(frame_end, ctx);
}

void dttr_mods_imgui_begin(const DTTR_Mods_RenderContext *ctx) {
	MOD_DISPATCH(imgui_begin, ctx);
}

void dttr_mods_imgui_end(const DTTR_Mods_RenderContext *ctx) {
	MOD_DISPATCH(imgui_end, ctx);
}

void dttr_mods_overlay_visible_changed(bool visible) {
	MOD_DISPATCH(overlay_visible_changed, visible);
}

void dttr_mods_window_created(const DTTR_Mods_WindowContext *ctx) {
	MOD_DISPATCH(window_created, ctx);
}

void dttr_mods_window_resized(const DTTR_Mods_WindowContext *ctx) {
	MOD_DISPATCH(window_resized, ctx);
}

void dttr_mods_window_destroying(const DTTR_Mods_WindowContext *ctx) {
	MOD_DISPATCH(window_destroying, ctx);
}

void dttr_mods_graphics_device_created(const DTTR_Mods_GraphicsContext *ctx) {
	MOD_DISPATCH(graphics_device_created, ctx);
}

void dttr_mods_graphics_device_lost(const DTTR_Mods_GraphicsContext *ctx) {
	MOD_DISPATCH(graphics_device_lost, ctx);
}

void dttr_mods_graphics_device_restored(const DTTR_Mods_GraphicsContext *ctx) {
	MOD_DISPATCH(graphics_device_restored, ctx);
}

void dttr_mods_graphics_device_destroying(const DTTR_Mods_GraphicsContext *ctx) {
	MOD_DISPATCH(graphics_device_destroying, ctx);
}

static bool dispatch_event_until_consumed(const SDL_Event *event, bool before_event) {
	for (size_t i = 0; i < kv_size(loaded_mods); i++) {
		loaded_mod *mod = &kv_A(loaded_mods, i);
		DTTR_Mods_EventFn event_fn = before_event ? mod->before_event : mod->event;
		if (!event_fn) {
			continue;
		}

		bool consumed = false;
		MOD_WITH_OWNER(mod, consumed = event_fn(event));
		if (consumed) {
			return true;
		}
	}

	return false;
}

bool dttr_mods_before_event(const SDL_Event *event) {
	return dispatch_event_until_consumed(event, true);
}

void dttr_mods_after_event(const SDL_Event *event, bool consumed) {
	MOD_DISPATCH(after_event, event, consumed);
}

void dttr_mods_input_mode_changed(const DTTR_Mods_InputContext *ctx) {
	MOD_DISPATCH(input_mode_changed, ctx);
}

bool dttr_mods_should_advance_game_frame() {
	for (size_t i = 0; i < kv_size(loaded_mods); i++) {
		loaded_mod *mod = &kv_A(loaded_mods, i);
		if (!mod->should_advance_game_frame) {
			continue;
		}

		bool should_advance = true;
		MOD_WITH_OWNER(mod, should_advance = mod->should_advance_game_frame());
		if (!should_advance) {
			return false;
		}
	}

	return true;
}

void dttr_mods_game_frame_advanced() { MOD_DISPATCH(game_frame_advanced); }

void dttr_mods_game_frame_blocked() { MOD_DISPATCH(game_frame_blocked); }

bool dttr_mods_has_render_game() {
	for (size_t i = 0; i < kv_size(loaded_mods); i++) {
		if (kv_A(loaded_mods, i).render_game) {
			return true;
		}
	}

	return false;
}

void dttr_mods_render_game(const DTTR_Mods_RenderGameContext *ctx) {
	MOD_DISPATCH(render_game, ctx);
}

void dttr_mods_render(const DTTR_Mods_RenderContext *ctx) { MOD_DISPATCH(render, ctx); }

bool dttr_mods_handle_event(const SDL_Event *event) {
	return dispatch_event_until_consumed(event, false);
}

size_t dttr_mods_loaded_count() { return kv_size(loaded_mods); }

const char *dttr_mods_loaded_name(size_t index) {
	if (index >= kv_size(loaded_mods)) {
		return NULL;
	}

	loaded_mod *mod = &kv_A(loaded_mods, index);
	return mod->display_name[0] ? mod->display_name : mod->filename;
}

DWORD dttr_mods_loaded_elapsed_ms(size_t index) {
	if (index >= kv_size(loaded_mods)) {
		return 0;
	}

	return GetTickCount() - kv_A(loaded_mods, index).loaded_at_ms;
}

bool dttr_mods_hot_reload_enabled() { return dttr_config.hot_reload; }

void dttr_mods_cleanup() {
	remove_all_mods(false);
	kv_destroy(loaded_mods);
	kv_init(loaded_mods);
	mods_dir[0] = '\0';
	last_reload_scan_ms = 0;
}
