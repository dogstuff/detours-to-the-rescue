#include <dttr_config.h>

#include "../graphics/graphics_private.h"
#include "context_private.h"
#include "mods_private.h"
#include "sidecar_private.h"
#include <dttr_runtime.h>

#include <dttr_errors.h>
#include <dttr_log.h>
#include <dttr_mod_config.h>
#include <dttr_path.h>

#include <kvec.h>
#include <sds.h>

#include <stdlib.h>

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
		.window = dttr_graphics_get_window(),
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
	return SDL_strncmp(
			   filename,
			   DTTR_MODS_SHADOW_PREFIX,
			   SDL_strlen(DTTR_MODS_SHADOW_PREFIX)
		   )
		   == 0;
}

static const DTTR_Mods_Info *get_mod_info(DTTR_Mods_InfoFn info_fn) {
	return info_fn ? info_fn() : NULL;
}

static void set_mod_display_name(loaded_mod *mod, const DTTR_Mods_Info *info) {
	const char *name = info && info->name && info->name[0] ? info->name : mod->filename;
	DTTR_Path_CopyString(mod->display_name, sizeof(mod->display_name), name);
}

static void set_mod_display_version(loaded_mod *mod, const DTTR_Mods_Info *info) {
	const char *v = info && info->version && info->version[0] ? info->version : "?";

	DTTR_Path_CopyString(mod->display_version, sizeof(mod->display_version), v);
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

#define MOD_DISPATCH_REVERSE(field, ...)                                                 \
	do {                                                                                 \
		for (size_t i = kv_size(loaded_mods); i > 0; i--) {                              \
			loaded_mod *mod = &kv_A(loaded_mods, i - 1);                                 \
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
	X(query_timing_policy, DTTR_Mods_QueryTimingPolicyFn, "DTTR_Mod_QueryTimingPolicy")  \
	X(timing_host_frame_begin,                                                           \
	  DTTR_Mods_TimingHostFrameBeginFn,                                                  \
	  "DTTR_Mod_TimingHostFrameBegin")                                                   \
	X(timing_should_run_simulation_step,                                                 \
	  DTTR_Mods_TimingShouldRunSimulationStepFn,                                         \
	  "DTTR_Mod_TimingShouldRunSimulationStep")                                          \
	X(timing_before_simulation_step,                                                     \
	  DTTR_Mods_TimingBeforeSimulationStepFn,                                            \
	  "DTTR_Mod_TimingBeforeSimulationStep")                                             \
	X(timing_after_simulation_step,                                                      \
	  DTTR_Mods_TimingAfterSimulationStepFn,                                             \
	  "DTTR_Mod_TimingAfterSimulationStep")                                              \
	X(timing_simulation_step_deferred,                                                   \
	  DTTR_Mods_TimingSimulationStepDeferredFn,                                          \
	  "DTTR_Mod_TimingSimulationStepDeferred")                                           \
	X(timing_before_render_frame,                                                        \
	  DTTR_Mods_TimingBeforeRenderFrameFn,                                               \
	  "DTTR_Mod_TimingBeforeRenderFrame")                                                \
	X(timing_after_render_frame,                                                         \
	  DTTR_Mods_TimingAfterRenderFrameFn,                                                \
	  "DTTR_Mod_TimingAfterRenderFrame")                                                 \
	X(timing_before_present_frame,                                                       \
	  DTTR_Mods_TimingBeforePresentFrameFn,                                              \
	  "DTTR_Mod_TimingBeforePresentFrame")                                               \
	X(timing_after_present_frame,                                                        \
	  DTTR_Mods_TimingAfterPresentFrameFn,                                               \
	  "DTTR_Mod_TimingAfterPresentFrame")                                                \
	X(timing_host_frame_end,                                                             \
	  DTTR_Mods_TimingHostFrameEndFn,                                                    \
	  "DTTR_Mod_TimingHostFrameEnd")                                                     \
	X(game_frame_advanced, DTTR_Mods_GameFrameAdvancedFn, "DTTR_Mod_GameFrameAdvanced")  \
	X(config, DTTR_Mods_ConfigFn, "DTTR_Mod_Config")

static void delete_shadow_copy(loaded_mod *mod) {
	if (!mod->shadow_path[0]) {
		return;
	}

	DeleteFileA(mod->shadow_path);
	mod->shadow_path[0] = '\0';
}

static int find_mod(const char *filename) {
	for (size_t i = 0; i < kv_size(loaded_mods); i++) {
		if (SDL_strcmp(kv_A(loaded_mods, i).filename, filename) == 0) {
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
	SDL_memset(out, 0, sizeof(*out));
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

#define DTTR_MOD_FIELD_HAS(field, member)                                                \
	((field)->struct_size                                                                \
	 >= offsetof(DTTR_Mods_ConfigField, member) + sizeof((field)->member))

static void clamp_seeded_field_range(
	const char *mod_id,
	const DTTR_Mods_ConfigField *field
) {
	if (field->type == DTTR_MODS_CONFIG_FIELD_INT && DTTR_MOD_FIELD_HAS(field, int_max)
		&& field->int_min < field->int_max) {
		int value;
		if (DTTR_Config_GetModInt(&dttr_config, mod_id, field->id, &value).status
			== DTTR_OK) {
			if (value < field->int_min) {
				DTTR_Config_SetModInt(&dttr_config, mod_id, field->id, field->int_min);
			} else if (value > field->int_max) {
				DTTR_Config_SetModInt(&dttr_config, mod_id, field->id, field->int_max);
			}
		}
	} else if (
		field->type == DTTR_MODS_CONFIG_FIELD_FLOAT
		&& DTTR_MOD_FIELD_HAS(field, float_max) && field->float_min < field->float_max
	) {
		float value;
		if (DTTR_Config_GetModFloat(&dttr_config, mod_id, field->id, &value).status
			== DTTR_OK) {
			if (value < field->float_min) {
				DTTR_Config_SetModFloat(&dttr_config, mod_id, field->id, field->float_min);
			} else if (value > field->float_max) {
				DTTR_Config_SetModFloat(&dttr_config, mod_id, field->id, field->float_max);
			}
		}
	}
}

#undef DTTR_MOD_FIELD_HAS

static void seed_mod_config_field(const char *mod_id, const DTTR_Mods_ConfigField *field) {
	if (!DTTR_Mods_ConfigField_ValidScalar(field)
		|| (field->type == DTTR_MODS_CONFIG_FIELD_ENUM
			&& !DTTR_Mods_ConfigField_Valid(field))) {
		return;
	}

	DTTR_ConfigModDefault def;
	if (DTTR_ModConfig_ResolveFieldDefault(
			field,
			DTTR_ModConfig_StringDefault(field),
			&def
		)
			.status
		!= DTTR_OK) {
		return;
	}

	DTTR_Result result = DTTR_Config_ApplyModFieldDefault(
		&dttr_config,
		mod_id,
		field->id,
		&def,
		false
	);
	if (!DTTR_ResultOK(result)) {
		DTTR_LOG_WARN(
			"Mod %s field %s: default not applied (%s: %s)",
			mod_id,
			field->id,
			DTTR_StatusName(result.status),
			dttr_sidecar_result_detail(result)
		);
		return;
	}

	clamp_seeded_field_range(mod_id, field);
}

// Seeds a mod's declared field defaults where a value is absent, so runtime reads
// return spec defaults even before DttR Config is opened.
static void seed_mod_config_defaults(const loaded_mod *mod) {
	if (!mod || !mod->config) {
		return;
	}

	const DTTR_Mods_ConfigSpec *spec = mod->config();
	if (!DTTR_Mods_ConfigSpec_Valid(spec)) {
		return;
	}

	if (spec->schema_version > 0) {
		uint32_t existing;
		if (DTTR_Config_GetModSchemaVersion(&dttr_config, spec->mod_id, &existing).status
			!= DTTR_OK) {
			DTTR_Config_SetModSchemaVersion(
				&dttr_config,
				spec->mod_id,
				spec->schema_version
			);
		}
	}

	for (size_t i = 0; i < spec->field_count; i++) {
		seed_mod_config_field(spec->mod_id, &spec->fields[i]);
	}
}

// Calls mod initialization and records ownership for hooks installed by that DLL.
static bool init_mod(loaded_mod *mod) {
	const DTTR_Mods_Info *info = get_mod_info(mod->info);
	set_mod_display_name(mod, info);
	set_mod_display_version(mod, info);
	log_mod_info(mod->filename, info);

	const DTTR_Mods_Context *base_ctx = dttr_sidecar_context();
	if (!refresh_mod_context(mod, base_ctx)) {
		unload_mod(mod);
		return false;
	}

	seed_mod_config_defaults(mod);

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

void dttr_mods_late_init() {
	MOD_DISPATCH(late_init);
}

void dttr_mods_frame_begin(const DTTR_Mods_FrameContext *ctx) {
	MOD_DISPATCH(frame_begin, ctx);
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

static bool ratio_is_set(DTTR_Mods_RatioU32 ratio) {
	return ratio.num != 0 && ratio.den != 0;
}

static int compare_ratio(DTTR_Mods_RatioU32 lhs, DTTR_Mods_RatioU32 rhs) {
	const uint64_t lhs_scaled = (uint64_t)lhs.num * rhs.den;
	const uint64_t rhs_scaled = (uint64_t)rhs.num * lhs.den;
	return (lhs_scaled > rhs_scaled) - (lhs_scaled < rhs_scaled);
}

static DTTR_Mods_RatioU32 max_ratio(DTTR_Mods_RatioU32 lhs, DTTR_Mods_RatioU32 rhs) {
	if (!ratio_is_set(lhs)) {
		return rhs;
	}

	if (!ratio_is_set(rhs)) {
		return lhs;
	}

	return compare_ratio(lhs, rhs) >= 0 ? lhs : rhs;
}

static DTTR_Mods_RatioU32 min_ratio(DTTR_Mods_RatioU32 lhs, DTTR_Mods_RatioU32 rhs) {
	if (!ratio_is_set(lhs)) {
		return rhs;
	}

	if (!ratio_is_set(rhs)) {
		return lhs;
	}

	return compare_ratio(lhs, rhs) <= 0 ? lhs : rhs;
}

static uint32_t min_nonzero_u32(uint32_t lhs, uint32_t rhs) {
	if (!lhs) {
		return rhs;
	}

	if (!rhs) {
		return lhs;
	}

	return lhs < rhs ? lhs : rhs;
}

static uint64_t min_nonzero_u64(uint64_t lhs, uint64_t rhs) {
	if (!lhs) {
		return rhs;
	}

	if (!rhs) {
		return lhs;
	}

	return lhs < rhs ? lhs : rhs;
}

bool dttr_mods_select_timing_policy(DTTR_Mods_TimingPolicyRequest *out_policy) {
	if (!out_policy || out_policy->struct_size < sizeof(*out_policy)) {
		return false;
	}

	DTTR_Mods_TimingPolicyRequest selected = {
		.struct_size = sizeof(DTTR_Mods_TimingPolicyRequest),
		.abi_version = DTTR_SDK_ABI_VERSION,
		.mode = DTTR_MODS_TIMING_NATIVE,
	};

	for (size_t i = 0; i < kv_size(loaded_mods); i++) {
		loaded_mod *mod = &kv_A(loaded_mods, i);
		if (!mod->query_timing_policy) {
			continue;
		}

		DTTR_Mods_TimingPolicyRequest request = {
			.struct_size = sizeof(DTTR_Mods_TimingPolicyRequest),
			.abi_version = DTTR_SDK_ABI_VERSION,
		};
		bool requested = false;
		MOD_WITH_OWNER(mod, requested = mod->query_timing_policy(&request));
		if (!requested || request.mode == DTTR_MODS_TIMING_NATIVE) {
			continue;
		}

		if (request.struct_size < offsetof(DTTR_Mods_TimingPolicyRequest, reserved)
			|| request.mode != DTTR_MODS_TIMING_FIXED_SIM_VARIABLE_RENDER) {
			DTTR_LOG_ERROR("Mod %s returned an invalid timing policy", mod->filename);
			return false;
		}

		selected.mode = DTTR_MODS_TIMING_FIXED_SIM_VARIABLE_RENDER;
		selected.min_sim_hz = max_ratio(selected.min_sim_hz, request.min_sim_hz);
		selected.max_sim_hz = min_ratio(selected.max_sim_hz, request.max_sim_hz);
		selected.preferred_sim_hz = max_ratio(
			selected.preferred_sim_hz,
			request.preferred_sim_hz
		);
		selected.max_sim_steps_per_host_frame = min_nonzero_u32(
			selected.max_sim_steps_per_host_frame,
			request.max_sim_steps_per_host_frame
		);
		selected.max_host_delta_ns = min_nonzero_u64(
			selected.max_host_delta_ns,
			request.max_host_delta_ns
		);
		selected.max_accumulator_debt_ns = min_nonzero_u64(
			selected.max_accumulator_debt_ns,
			request.max_accumulator_debt_ns
		);
	}

	if (selected.mode == DTTR_MODS_TIMING_FIXED_SIM_VARIABLE_RENDER) {
		if (ratio_is_set(selected.min_sim_hz) && ratio_is_set(selected.max_sim_hz)
			&& compare_ratio(selected.min_sim_hz, selected.max_sim_hz) > 0) {
			DTTR_LOG_ERROR("Conflicting DTTR fixed timing policy ranges");
			return false;
		}

		if (!ratio_is_set(selected.preferred_sim_hz)) {
			selected.preferred_sim_hz = ratio_is_set(selected.min_sim_hz)
											? selected.min_sim_hz
											: selected.max_sim_hz;
		}

		if (ratio_is_set(selected.min_sim_hz)
			&& compare_ratio(selected.preferred_sim_hz, selected.min_sim_hz) < 0) {
			selected.preferred_sim_hz = selected.min_sim_hz;
		}

		if (ratio_is_set(selected.max_sim_hz)
			&& compare_ratio(selected.preferred_sim_hz, selected.max_sim_hz) > 0) {
			selected.preferred_sim_hz = selected.max_sim_hz;
		}
	}

	*out_policy = selected;
	return true;
}

void dttr_mods_timing_host_frame_begin(const DTTR_Mods_TimingFrameState *ctx) {
	MOD_DISPATCH(timing_host_frame_begin, ctx);
}

bool dttr_mods_timing_should_run_simulation_step(const DTTR_Mods_TimingFrameState *ctx) {
	for (size_t i = 0; i < kv_size(loaded_mods); i++) {
		loaded_mod *mod = &kv_A(loaded_mods, i);
		if (!mod->timing_should_run_simulation_step) {
			continue;
		}

		bool should_run = true;
		MOD_WITH_OWNER(mod, should_run = mod->timing_should_run_simulation_step(ctx));
		if (!should_run) {
			return false;
		}
	}

	return true;
}

void dttr_mods_timing_before_simulation_step(const DTTR_Mods_TimingFrameState *ctx) {
	MOD_DISPATCH(timing_before_simulation_step, ctx);
}

void dttr_mods_timing_after_simulation_step(const DTTR_Mods_TimingFrameState *ctx) {
	MOD_DISPATCH_REVERSE(timing_after_simulation_step, ctx);
}

void dttr_mods_timing_simulation_step_deferred(const DTTR_Mods_TimingFrameState *ctx) {
	MOD_DISPATCH(timing_simulation_step_deferred, ctx);
}

void dttr_mods_timing_before_render_frame(const DTTR_Mods_TimingFrameState *ctx) {
	MOD_DISPATCH(timing_before_render_frame, ctx);
}

void dttr_mods_timing_after_render_frame(const DTTR_Mods_TimingFrameState *ctx) {
	MOD_DISPATCH_REVERSE(timing_after_render_frame, ctx);
}

void dttr_mods_timing_before_present_frame(const DTTR_Mods_TimingFrameState *ctx) {
	MOD_DISPATCH(timing_before_present_frame, ctx);
}

void dttr_mods_timing_after_present_frame(const DTTR_Mods_TimingFrameState *ctx) {
	MOD_DISPATCH_REVERSE(timing_after_present_frame, ctx);
}

void dttr_mods_timing_host_frame_end(const DTTR_Mods_TimingFrameState *ctx) {
	MOD_DISPATCH_REVERSE(timing_host_frame_end, ctx);
}

void dttr_mods_game_frame_advanced() {
	MOD_DISPATCH(game_frame_advanced);
}

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

void dttr_mods_render(const DTTR_Mods_RenderContext *ctx) {
	MOD_DISPATCH(render, ctx);
}

bool dttr_mods_handle_event(const SDL_Event *event) {
	return dispatch_event_until_consumed(event, false);
}

size_t dttr_mods_loaded_count() {
	return kv_size(loaded_mods);
}

const char *dttr_mods_loaded_name(size_t index) {
	if (index >= kv_size(loaded_mods)) {
		return NULL;
	}

	loaded_mod *mod = &kv_A(loaded_mods, index);
	return mod->display_name[0] ? mod->display_name : mod->filename;
}

const char *dttr_mods_loaded_version(size_t index) {
	if (index >= kv_size(loaded_mods)) {
		return NULL;
	}

	return kv_A(loaded_mods, index).display_version;
}

DWORD dttr_mods_loaded_elapsed_ms(size_t index) {
	if (index >= kv_size(loaded_mods)) {
		return 0;
	}

	return GetTickCount() - kv_A(loaded_mods, index).loaded_at_ms;
}

bool dttr_mods_hot_reload_enabled() {
	return dttr_config.hot_reload;
}

void dttr_mods_cleanup() {
	remove_all_mods(false);
	kv_destroy(loaded_mods);
	kv_init(loaded_mods);
	mods_dir[0] = '\0';
	last_reload_scan_ms = 0;
}
