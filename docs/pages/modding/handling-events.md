# Handling Mod Callbacks

Callbacks define how your mod's code is actually executed. Avoid using a lower-level render or window callback when a simpler callback is enough.

## Setup and cleanup

Every mod should start with `DTTR_MODS_INIT` and clean up with `DTTR_MODS_CLEANUP`. Most simple mods only need those two lifecycle callbacks.

Use `DTTR_MODS_INIT` for work that must happen once when the mod loads:

- Save the `ctx` pointer if later callbacks need it.
- Check that the game functions or globals you need are available.
- Install hooks or patches.
- Create any mod-owned state.
- Open files or load configuration.

Return `false` from `DTTR_MODS_INIT` if the mod cannot safely run. For example, fail init if a required game symbol is missing or a required patch cannot be installed. DttR will not keep running a mod that failed to initialize.

Use `DTTR_MODS_CLEANUP` to undo anything your mod set up:

- Release patch groups.
- Uninstall hooks.
- Free memory owned by the mod.
- Close files and handles.
- Reset pointers to mod-owned objects if they might be reused during shutdown.

`DTTR_MODS_LATE_INIT` is for setup that needs all mods to be loaded first. Use it when your mod wants to discover or coordinate with another mod. If your mod only touches DttR or its own state, use `DTTR_MODS_INIT` instead.

`DTTR_MODS_BEFORE_UNLOAD` is a final warning that unloading is about to start. Use it for last notifications or coordination between mods. Do not put ordinary cleanup there; keep ordinary cleanup in `DTTR_MODS_CLEANUP`.

## Per-frame work

Use these for regular updates:

- `DTTR_MODS_TICK`: General periodic work.
- `DTTR_MODS_FRAME_BEGIN`: Work at the start of a DttR frame.
- `DTTR_MODS_FRAME_END`: Work after the frame finishes.

Keep these callbacks light. If a task can be done once during init, do it during init instead of every frame.

## Input and events

Use these for SDL events, hotkeys, diagnostics, and input blocking:

- `DTTR_MODS_BEFORE_EVENT`: Inspect or consume an event before the game receives it.
- `DTTR_MODS_AFTER_EVENT`: Inspect the event after DttR and earlier callbacks have processed it.
- `DTTR_MODS_INPUT_MODE_CHANGED`: React when input mode changes.

`DTTR_MODS_BEFORE_EVENT` can block delivery to the game. `DTTR_MODS_AFTER_EVENT` receives the final consumed state.

## Drawing

Use these for custom drawing:

- `DTTR_MODS_RENDER_GAME`: Draw at the game's internal resolution.
- `DTTR_MODS_RENDER`: Draw at the full window resolution.

Use `DTTR_MODS_RENDER_GAME` for things that should line up with the game view. Use `DTTR_MODS_RENDER` for full-window overlays.

## Render timing

Use these only when your mod needs exact render-backend timing:

- `DTTR_MODS_BEFORE_GAME_FRAME`
- `DTTR_MODS_AFTER_GAME_FRAME`
- `DTTR_MODS_BEFORE_PRESENT`
- `DTTR_MODS_AFTER_PRESENT`

Most mods do not need these. Prefer the simpler frame or drawing callbacks first.

## Window and graphics lifetime

Use these when your mod owns graphics resources that must follow the window:

- `DTTR_MODS_WINDOW_CREATED`: Create resources that need the SDL window.
- `DTTR_MODS_WINDOW_RESIZED`: Update size-dependent state after the window changes size.
- `DTTR_MODS_WINDOW_DESTROYING`: Release anything tied directly to the window before it is destroyed.

Use these when your mod owns graphics resources that must follow the graphics device:

- `DTTR_MODS_GRAPHICS_DEVICE_CREATED`: Create device-dependent resources.
- `DTTR_MODS_GRAPHICS_DEVICE_LOST`: Stop using device resources after the device is lost.
- `DTTR_MODS_GRAPHICS_DEVICE_RESTORED`: Recreate or refresh resources after the device comes back.
- `DTTR_MODS_GRAPHICS_DEVICE_DESTROYING`: Release device resources before shutdown.

Create and destroy device-dependent resources in the matching lifetime callbacks. Do not assume the window or graphics device lives for the entire mod lifetime.

## Pausing the game

Use these when an overlay or tool needs the window to keep presenting while the game pauses:

- `DTTR_MODS_SHOULD_ADVANCE_GAME_FRAME`: Decide whether the game should advance.
- `DTTR_MODS_GAME_FRAME_ADVANCED`: Run work after a game step advances.
- `DTTR_MODS_GAME_FRAME_BLOCKED`: Run work when presentation continues but the game does not advance.

Use `DTTR_MODS_GAME_FRAME_ADVANCED` for conditional game-step logic. Render callbacks may still run while game advancement is blocked.
