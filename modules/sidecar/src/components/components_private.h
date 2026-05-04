#ifndef COMPONENTS_PRIVATE_H
#define COMPONENTS_PRIVATE_H

#include <stdbool.h>
#include <windows.h>

#include <SDL3/SDL.h>
#include <dttr_components.h>

void dttr_components_init(void);
void dttr_components_tick(void);
bool dttr_components_should_advance_game_frame(void);
void dttr_components_game_frame_advanced(void);
void dttr_components_game_frame_blocked(void);
bool dttr_components_has_render_game(void);
void dttr_components_render_game(const DTTR_RenderGameContext *ctx);
void dttr_components_render(const DTTR_RenderContext *ctx);
bool dttr_components_handle_event(const SDL_Event *event);
void dttr_components_cleanup(void);

#define S_COMPONENTS_MAX 32u

typedef struct {
	FILETIME m_write_time;
	DWORD m_size_high;
	DWORD m_size_low;
} S_ComponentFileId;

typedef struct {
	HMODULE m_handle;
	void *m_hook_owner;
	char m_filename[MAX_PATH];
	char m_source_path[MAX_PATH];
	char m_shadow_path[MAX_PATH];
	S_ComponentFileId m_source_file;
	S_ComponentFileId m_pending_file;
	DTTR_ComponentInitFn m_init;
	DTTR_ComponentCleanupFn m_cleanup;
	DTTR_ComponentTickFn m_tick;
	DTTR_ComponentEventFn m_event;
	DTTR_ComponentInfoFn m_info;
	DTTR_ComponentRenderGameFn m_render_game;
	DTTR_ComponentRenderFn m_render;
	DTTR_ComponentShouldAdvanceGameFrameFn m_should_advance_game_frame;
	DTTR_ComponentGameFrameAdvancedFn m_game_frame_advanced;
	DTTR_ComponentGameFrameBlockedFn m_game_frame_blocked;
	DWORD m_pending_since_ms;
	bool m_reload_pending;
	bool m_initialized;
} S_LoadedComponent;

#endif /* COMPONENTS_PRIVATE_H */
