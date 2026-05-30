#ifndef MODS_PRIVATE_H
#define MODS_PRIVATE_H

#include <stdbool.h>
#include <stddef.h>
#include <windows.h>

#include <SDL3/SDL.h>
#include <dttr_mods.h>

void dttr_mods_init();
void dttr_mods_tick();
void dttr_mods_late_init();
void dttr_mods_before_unload();
void dttr_mods_frame_begin(const DTTR_Mods_FrameContext *ctx);
void dttr_mods_before_game_frame(const DTTR_Mods_FrameContext *ctx);
void dttr_mods_after_game_frame(const DTTR_Mods_FrameContext *ctx);
void dttr_mods_before_present(const DTTR_Mods_PresentContext *ctx);
void dttr_mods_after_present(const DTTR_Mods_PresentContext *ctx);
void dttr_mods_frame_end(const DTTR_Mods_FrameContext *ctx);
void dttr_mods_imgui_begin(const DTTR_Mods_RenderContext *ctx);
void dttr_mods_imgui_end(const DTTR_Mods_RenderContext *ctx);
void dttr_mods_overlay_visible_changed(bool visible);
void dttr_mods_window_created(const DTTR_Mods_WindowContext *ctx);
void dttr_mods_window_resized(const DTTR_Mods_WindowContext *ctx);
void dttr_mods_window_destroying(const DTTR_Mods_WindowContext *ctx);
void dttr_mods_graphics_device_created(const DTTR_Mods_GraphicsContext *ctx);
void dttr_mods_graphics_device_lost(const DTTR_Mods_GraphicsContext *ctx);
void dttr_mods_graphics_device_restored(const DTTR_Mods_GraphicsContext *ctx);
void dttr_mods_graphics_device_destroying(const DTTR_Mods_GraphicsContext *ctx);
bool dttr_mods_before_event(const SDL_Event *event);
void dttr_mods_after_event(const SDL_Event *event, bool consumed);
void dttr_mods_input_mode_changed(const DTTR_Mods_InputContext *ctx);
bool dttr_mods_should_advance_game_frame();
void dttr_mods_game_frame_advanced();
void dttr_mods_game_frame_blocked();
bool dttr_mods_has_render_game();
void dttr_mods_render_game(const DTTR_Mods_RenderGameContext *ctx);
void dttr_mods_render(const DTTR_Mods_RenderContext *ctx);
bool dttr_mods_handle_event(const SDL_Event *event);
size_t dttr_mods_loaded_count();
const char *dttr_mods_loaded_name(size_t index);
DWORD dttr_mods_loaded_elapsed_ms(size_t index);
bool dttr_mods_hot_reload_enabled();
void dttr_mods_cleanup();

#define MODS_MAX 32u

typedef struct {
	FILETIME write_time;
	DWORD size_high;
	DWORD size_low;
} mod_file_id;

typedef struct {
	HMODULE handle;
	void *hook_owner;
	DTTR_Mods_Context *context;
	char filename[MAX_PATH];
	char source_path[MAX_PATH];
	char shadow_path[MAX_PATH];
	char display_name[MAX_PATH];
	mod_file_id source_file;
	mod_file_id pending_file;
	DTTR_Mods_InitFn init;
	DTTR_Mods_CleanupFn cleanup;
	DTTR_Mods_TickFn tick;
	DTTR_Mods_EventFn event;
	DTTR_Mods_InfoFn info;
	DTTR_Mods_LateInitFn late_init;
	DTTR_Mods_BeforeUnloadFn before_unload;
	DTTR_Mods_FrameBeginFn frame_begin;
	DTTR_Mods_BeforeGameFrameFn before_game_frame;
	DTTR_Mods_AfterGameFrameFn after_game_frame;
	DTTR_Mods_BeforePresentFn before_present;
	DTTR_Mods_AfterPresentFn after_present;
	DTTR_Mods_FrameEndFn frame_end;
	DTTR_Mods_ImGuiBeginFn imgui_begin;
	DTTR_Mods_ImGuiEndFn imgui_end;
	DTTR_Mods_OverlayVisibleChangedFn overlay_visible_changed;
	DTTR_Mods_WindowCreatedFn window_created;
	DTTR_Mods_WindowResizedFn window_resized;
	DTTR_Mods_WindowDestroyingFn window_destroying;
	DTTR_Mods_GraphicsDeviceCreatedFn graphics_device_created;
	DTTR_Mods_GraphicsDeviceLostFn graphics_device_lost;
	DTTR_Mods_GraphicsDeviceRestoredFn graphics_device_restored;
	DTTR_Mods_GraphicsDeviceDestroyingFn graphics_device_destroying;
	DTTR_Mods_BeforeEventFn before_event;
	DTTR_Mods_AfterEventFn after_event;
	DTTR_Mods_InputModeChangedFn input_mode_changed;
	DTTR_Mods_RenderGameFn render_game;
	DTTR_Mods_RenderFn render;
	DTTR_Mods_ShouldAdvanceGameFrameFn should_advance_game_frame;
	DTTR_Mods_GameFrameAdvancedFn game_frame_advanced;
	DTTR_Mods_GameFrameBlockedFn game_frame_blocked;
	DWORD pending_since_ms;
	DWORD loaded_at_ms;
	bool reload_pending;
	bool initialized;
} loaded_mod;

#endif // MODS_PRIVATE_H
