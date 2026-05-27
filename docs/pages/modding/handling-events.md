# Handling Mod Callbacks

Callbacks are the main mod entry points. Keep examples in `examples/mod-example`; this page only maps callback families to use cases.

## Choose the Smallest Callback

- Use `DTTR_MODS_INIT` and `DTTR_MODS_CLEANUP` for setup and teardown.
- Use `DTTR_MODS_LATE_INIT` and `DTTR_MODS_BEFORE_UNLOAD` for cross-mod setup or final unload notifications.
- Use `DTTR_MODS_TICK`, `DTTR_MODS_FRAME_BEGIN`, and `DTTR_MODS_FRAME_END` for regular state updates.
- Use `DTTR_MODS_BEFORE_EVENT`, `DTTR_MODS_AFTER_EVENT`, and `DTTR_MODS_INPUT_MODE_CHANGED` for input, hotkeys, blocking, and diagnostics.
- Use `DTTR_MODS_IMGUI_BEGIN` and `DTTR_MODS_IMGUI_END` for ImGui widgets.
- Use `DTTR_MODS_RENDER_GAME` for game-resolution drawing and `DTTR_MODS_RENDER` for full-window drawing.
- Use `DTTR_MODS_BEFORE_GAME_FRAME`, `DTTR_MODS_AFTER_GAME_FRAME`, `DTTR_MODS_BEFORE_PRESENT`, and `DTTR_MODS_AFTER_PRESENT` for render-backend timing.
- Use `DTTR_MODS_WINDOW_*` and `DTTR_MODS_GRAPHICS_DEVICE_*` for window and graphics-device lifetime.
- Use `DTTR_MODS_SHOULD_ADVANCE_GAME_FRAME`, `DTTR_MODS_GAME_FRAME_ADVANCED`, and `DTTR_MODS_GAME_FRAME_BLOCKED` when overlays need to keep presenting while game simulation pauses.

## ImGui Ownership

DttR owns the ImGui context, frame flow, SDL backend, and renderer backend. Mods only submit widgets during the ImGui callbacks.

Do not call ImGui ownership functions from a mod:

- Context functions: `igCreateContext` / `igDestroyContext`.
- Frame functions: `igNewFrame` / `igRender`.
- Backend functions: `ImGui_ImplSDL3_*` or `ImGui_ImplOpenGL3_*` init/shutdown calls.

For SDL events, include `SDL3/SDL.h` and read callback data directly instead of forwarding events to ImGui backend APIs.

## Ordering Notes

`DTTR_MODS_BEFORE_EVENT` can consume an event before game delivery. `DTTR_MODS_AFTER_EVENT` receives the final consumed state.

Use `DTTR_MODS_GAME_FRAME_ADVANCED` for strict simulation-step work. Render-backend hooks can still run while game advancement is blocked but presentation continues.
