#include "components_private.h"
#include "dttr_sidecar.h"
#include "game_api_private.h"

#include <dttr_log.h>
#include <dttr_path.h>

#include <sds.h>

#include <stdio.h>
#include <string.h>

static S_LoadedComponent s_loaded_components[S_COMPONENTS_MAX];
static int s_loaded_component_count = 0;

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

void dttr_components_init(void) {
	DTTR_LOG_INFO("Loading components...");

	sds components_dir = sdsnew(g_dttr_loader_dir);
	if (!components_dir || !dttr_path_append_segment(&components_dir, "components", '\\')
		|| !dttr_path_append_separator(&components_dir, '\\')) {
		sdsfree(components_dir);
		return;
	}

	DWORD attrs = GetFileAttributesA(components_dir);
	if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
		DTTR_LOG_INFO("No components directory found at %s - skipping", components_dir);
		sdsfree(components_dir);
		return;
	}

	sds search_pattern = sdscat(sdsdup(components_dir), "*.dll");
	if (!search_pattern) {
		sdsfree(components_dir);
		return;
	}

	WIN32_FIND_DATAA find_data;
	HANDLE find_handle = FindFirstFileA(search_pattern, &find_data);
	sdsfree(search_pattern);

	if (find_handle == INVALID_HANDLE_VALUE) {
		DTTR_LOG_INFO("Loaded 0 component(s)");
		sdsfree(components_dir);
		return;
	}

	const DTTR_ComponentContext *base_ctx = dttr_game_api_get_ctx();

	do {
		if (s_loaded_component_count >= S_COMPONENTS_MAX) {
			DTTR_LOG_WARN(
				"Maximum component count (%d) reached - skipping remaining",
				S_COMPONENTS_MAX
			);
			break;
		}

		sds dll_path = sdscat(sdsdup(components_dir), find_data.cFileName);
		if (!dll_path) {
			break;
		}

		HMODULE handle = LoadLibraryA(dll_path);
		sdsfree(dll_path);

		if (!handle) {
			DTTR_LOG_WARN(
				"Failed to load component DLL: %s (error %lu)",
				find_data.cFileName,
				GetLastError()
			);
			continue;
		}

		DTTR_ComponentInitFn init_fn = (DTTR_ComponentInitFn)
			GetProcAddress(handle, "dttr_component_init");
		DTTR_ComponentCleanupFn cleanup_fn = (DTTR_ComponentCleanupFn)
			GetProcAddress(handle, "dttr_component_cleanup");

		if (!init_fn || !cleanup_fn) {
			DTTR_LOG_WARN(
				"Component %s missing required exports "
				"(dttr_component_init/dttr_component_cleanup) - skipping",
				find_data.cFileName
			);
			FreeLibrary(handle);
			continue;
		}

		S_LoadedComponent *component = &s_loaded_components[s_loaded_component_count];
		component->m_handle = handle;
		dttr_path_copy_string(
			component->m_filename,
			sizeof(component->m_filename),
			find_data.cFileName
		);
		component->m_init = init_fn;
		component->m_cleanup = cleanup_fn;
		component->m_tick = (DTTR_ComponentTickFn)
			GetProcAddress(handle, "dttr_component_tick");
		component->m_event = (DTTR_ComponentEventFn)
			GetProcAddress(handle, "dttr_component_event");
		component->m_info = (DTTR_ComponentInfoFn)
			GetProcAddress(handle, "dttr_component_info");
		component->m_render_game = (DTTR_ComponentRenderGameFn)
			GetProcAddress(handle, "dttr_component_render_game");
		component->m_render = (DTTR_ComponentRenderFn)
			GetProcAddress(handle, "dttr_component_render");

		s_log_component_info(find_data.cFileName, component->m_info);

		const DTTR_ComponentContext ctx = s_component_context(base_ctx);

		if (!component->m_init(&ctx)) {
			DTTR_LOG_WARN("Component %s init failed - skipping", component->m_filename);
			FreeLibrary(handle);
			continue;
		}

		component->m_initialized = true;
		s_loaded_component_count++;
	} while (FindNextFileA(find_handle, &find_data));

	FindClose(find_handle);
	sdsfree(components_dir);

	DTTR_LOG_INFO("Loaded %d component(s)", s_loaded_component_count);
}

void dttr_components_tick(void) {
	for (int i = 0; i < s_loaded_component_count; i++) {
		if (s_loaded_components[i].m_tick) {
			s_loaded_components[i].m_tick();
		}
	}
}

bool dttr_components_has_render_game(void) {
	for (int i = 0; i < s_loaded_component_count; i++) {
		if (s_loaded_components[i].m_render_game) {
			return true;
		}
	}
	return false;
}

void dttr_components_render_game(const DTTR_RenderGameContext *ctx) {
	for (int i = 0; i < s_loaded_component_count; i++) {
		if (s_loaded_components[i].m_render_game) {
			s_loaded_components[i].m_render_game(ctx);
		}
	}
}

void dttr_components_render(const DTTR_RenderContext *ctx) {
	for (int i = 0; i < s_loaded_component_count; i++) {
		if (s_loaded_components[i].m_render) {
			s_loaded_components[i].m_render(ctx);
		}
	}
}

bool dttr_components_handle_event(const SDL_Event *event) {
	for (int i = 0; i < s_loaded_component_count; i++) {
		if (s_loaded_components[i].m_event && s_loaded_components[i].m_event(event)) {
			return true;
		}
	}

	return false;
}

void dttr_components_cleanup(void) {
	for (int i = s_loaded_component_count - 1; i >= 0; i--) {
		S_LoadedComponent *component = &s_loaded_components[i];

		if (component->m_initialized) {
			DTTR_LOG_INFO("Cleaning up component: %s", component->m_filename);
			component->m_cleanup();
		}

		FreeLibrary(component->m_handle);
	}

	s_loaded_component_count = 0;
}
