#ifndef COMPONENTS_PRIVATE_H
#define COMPONENTS_PRIVATE_H

#include <stdbool.h>
#include <windows.h>

#include <SDL3/SDL.h>
#include <dttr_components.h>

void dttr_components_init(void);
void dttr_components_tick(void);
void dttr_components_late_init(void);
void dttr_components_before_unload(void);
void dttr_components_frame_begin(const DTTR_FrameContext *ctx);
void dttr_components_before_game_frame(const DTTR_FrameContext *ctx);
void dttr_components_after_game_frame(const DTTR_FrameContext *ctx);
void dttr_components_before_present(const DTTR_PresentContext *ctx);
void dttr_components_after_present(const DTTR_PresentContext *ctx);
void dttr_components_frame_end(const DTTR_FrameContext *ctx);
void dttr_components_imgui_begin(const DTTR_RenderContext *ctx);
void dttr_components_imgui_end(const DTTR_RenderContext *ctx);
void dttr_components_overlay_visible_changed(bool visible);
void dttr_components_window_created(const DTTR_WindowContext *ctx);
void dttr_components_window_resized(const DTTR_WindowContext *ctx);
void dttr_components_window_destroying(const DTTR_WindowContext *ctx);
void dttr_components_graphics_device_created(const DTTR_GraphicsContext *ctx);
void dttr_components_graphics_device_lost(const DTTR_GraphicsContext *ctx);
void dttr_components_graphics_device_restored(const DTTR_GraphicsContext *ctx);
void dttr_components_graphics_device_destroying(const DTTR_GraphicsContext *ctx);
bool dttr_components_before_event(const SDL_Event *event);
void dttr_components_after_event(const SDL_Event *event, bool consumed);
void dttr_components_input_mode_changed(const DTTR_InputContext *ctx);
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
	DTTR_ComponentLateInitFn m_late_init;
	DTTR_ComponentBeforeUnloadFn m_before_unload;
	DTTR_ComponentFrameBeginFn m_frame_begin;
	DTTR_ComponentBeforeGameFrameFn m_before_game_frame;
	DTTR_ComponentAfterGameFrameFn m_after_game_frame;
	DTTR_ComponentBeforePresentFn m_before_present;
	DTTR_ComponentAfterPresentFn m_after_present;
	DTTR_ComponentFrameEndFn m_frame_end;
	DTTR_ComponentImguiBeginFn m_imgui_begin;
	DTTR_ComponentImguiEndFn m_imgui_end;
	DTTR_ComponentOverlayVisibleChangedFn m_overlay_visible_changed;
	DTTR_ComponentWindowCreatedFn m_window_created;
	DTTR_ComponentWindowResizedFn m_window_resized;
	DTTR_ComponentWindowDestroyingFn m_window_destroying;
	DTTR_ComponentGraphicsDeviceCreatedFn m_graphics_device_created;
	DTTR_ComponentGraphicsDeviceLostFn m_graphics_device_lost;
	DTTR_ComponentGraphicsDeviceRestoredFn m_graphics_device_restored;
	DTTR_ComponentGraphicsDeviceDestroyingFn m_graphics_device_destroying;
	DTTR_ComponentBeforeEventFn m_before_event;
	DTTR_ComponentAfterEventFn m_after_event;
	DTTR_ComponentInputModeChangedFn m_input_mode_changed;
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
