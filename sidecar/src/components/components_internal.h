#ifndef DTTR_COMPONENTS_INTERNAL_H
#define DTTR_COMPONENTS_INTERNAL_H

#include <stdbool.h>
#include <windows.h>

#include <SDL3/SDL.h>
#include <dttr_components.h>

void dttr_components_init(void);
void dttr_components_tick(void);
void dttr_components_render(
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *render_target,
	uint32_t width,
	uint32_t height
);
bool dttr_components_handle_event(const SDL_Event *event);
void dttr_components_cleanup(void);

#define S_COMPONENTS_MAX 32

typedef struct {
	HMODULE m_handle;
	char m_filename[MAX_PATH];
	DTTR_ComponentInitFn m_init;
	DTTR_ComponentCleanupFn m_cleanup;
	DTTR_ComponentTickFn m_tick;
	DTTR_ComponentEventFn m_event;
	DTTR_ComponentInfoFn m_info;
	DTTR_ComponentRenderFn m_render;
	bool m_initialized;
} S_LoadedComponent;

#endif /* DTTR_COMPONENTS_INTERNAL_H */
