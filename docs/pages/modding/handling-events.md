# Handling Mod Callbacks

Use callbacks for most mod work: setup, per-frame updates, input handling, overlay drawing, and cleanup.

## Callback List

| Name | Behavior |
| --- | --- |
| `DTTR_MODS_LATE_INIT` | Runs after mods load for cross-mod setup or delayed checks. |
| `DTTR_MODS_BEFORE_UNLOAD` | Runs before a mod unloads, so you can do final cleanup. |
| `DTTR_MODS_TICK` | Runs each host tick for small bits of legacy tick work. Prefer frame callbacks when you need frame data. |
| `DTTR_MODS_FRAME_BEGIN` | Runs at the start of each host frame for timers, polling, and lightweight state updates. |
| `DTTR_MODS_BEFORE_GAME_FRAME` | Runs immediately before game image submission for render-backend work. This can also run on blocked presentation frames. |
| `DTTR_MODS_AFTER_GAME_FRAME` | Runs immediately after game image submission for render-backend work. This can also run on blocked presentation frames. |
| `DTTR_MODS_BEFORE_PRESENT` | Runs before command-buffer submit or buffer swap, after game and overlay draw/blit work has been queued. |
| `DTTR_MODS_AFTER_PRESENT` | Runs after presentation work for the host frame. |
| `DTTR_MODS_FRAME_END` | Runs at the end of each host frame after game-frame and presentation callbacks. |
| `DTTR_MODS_IMGUI_BEGIN` | Starts ImGui overlay work. |
| `DTTR_MODS_IMGUI_END` | Finishes ImGui overlay work. |
| `DTTR_MODS_RENDER_GAME` | Draws non-ImGui overlays at game resolution, letterboxed and scaled with the game image. |
| `DTTR_MODS_RENDER` | Draws non-ImGui overlays at full window resolution, including the area above the letterbox bars. |
| `DTTR_MODS_OVERLAY_VISIBLE_CHANGED` | Tracks overlay visibility for pause hotkeys, UI state, and input hints. |
| `DTTR_MODS_WINDOW_CREATED` | Runs when the host window is created. Use it for cached dimensions or platform handles. |
| `DTTR_MODS_WINDOW_RESIZED` | Runs when the host window is resized. Use it for cached dimensions or size-dependent resources. |
| `DTTR_MODS_WINDOW_DESTROYING` | Runs before the host window is destroyed. Release window-owned resources here. |
| `DTTR_MODS_GRAPHICS_DEVICE_CREATED` | Runs when the graphics device is created. Create device-owned resources here. |
| `DTTR_MODS_GRAPHICS_DEVICE_LOST` | Runs when the graphics device is lost. Pause or release invalid device-owned resources here. |
| `DTTR_MODS_GRAPHICS_DEVICE_RESTORED` | Runs when the graphics device is restored. Recreate device-owned resources here. |
| `DTTR_MODS_GRAPHICS_DEVICE_DESTROYING` | Runs before the graphics device is destroyed. Release device-owned resources here. |
| `DTTR_MODS_BEFORE_EVENT` | Observes input before normal processing and can consume events for hotkeys or input blocking. |
| `DTTR_MODS_AFTER_EVENT` | Observes input after normal processing with the final consumed state for diagnostics or bookkeeping. |
| `DTTR_MODS_EVENT` | Observes legacy single-callback event flow. |
| `DTTR_MODS_INPUT_MODE_CHANGED` | Reacts to game-input enable/disable behavior. |
| `DTTR_MODS_SHOULD_ADVANCE_GAME_FRAME` | Returns whether the host-loop game frame should advance while overlays keep presenting. |
| `DTTR_MODS_GAME_FRAME_ADVANCED` | Runs after a host loop frame advances the game. |
| `DTTR_MODS_GAME_FRAME_BLOCKED` | Runs after a host frame presents overlays without advancing the game. |

## Input Events

A before-event callback returns whether it consumed the event:

- If false, normal event processing continues
- If true, the game does not receive the event

Events move through the runtime in this order:

1. `DTTR_MODS_BEFORE_EVENT`, stopping at the first mod that consumes the event.
2. ImGui event processing.
3. Legacy `DTTR_MODS_EVENT`, also stopping at the first consuming mod.
4. Host movie/sidecar input handlers.
5. `DTTR_MODS_AFTER_EVENT`, dispatched to every mod with the final consumed state.

```c
#include <dttr_sdk.h>

static uint64_t seen_events;
static uint64_t consumed_events;

DTTR_MODS_BEFORE_EVENT {
    ++seen_events;
    return false;
}

DTTR_MODS_AFTER_EVENT {
    if (consumed) {
        ++consumed_events;
    }
}
```

## Per-Frame Work

Frame callbacks receive the current render-frame dimensions and scale. Use those values instead of guessing from cached window size.

```c
static uint64_t last_frame_index;
static uint32_t last_game_width;
static uint32_t last_game_height;

DTTR_MODS_FRAME_BEGIN {
    last_frame_index = ctx->frame_index;
    last_game_width = ctx->game_w;
    last_game_height = ctx->game_h;
}
```

## Game Frame Advancement

Some tools need to pause game execution while overlays keep presenting. `DTTR_MODS_SHOULD_ADVANCE_GAME_FRAME` controls whether that happens.

```c
static uint32_t blocked_frames_remaining;

DTTR_MODS_SHOULD_ADVANCE_GAME_FRAME {
    return blocked_frames_remaining == 0;
}

DTTR_MODS_GAME_FRAME_BLOCKED {
    if (blocked_frames_remaining > 0) {
        --blocked_frames_remaining;
    }
}
```

`DTTR_MODS_GAME_FRAME_ADVANCED` fires after a host-loop frame advances the game. `DTTR_MODS_GAME_FRAME_BLOCKED` fires after a frame presents without advancing it.

Use `DTTR_MODS_GAME_FRAME_ADVANCED` for strict simulation-step work. `DTTR_MODS_BEFORE_GAME_FRAME` and `DTTR_MODS_AFTER_GAME_FRAME` are render-backend image-submission hooks. They currently also run when advancement is blocked but presentation continues.
