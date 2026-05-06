# Component exports

DttR uses `GetProcAddress` to load the C exports below from each DLL in `components/`. A DLL must export the two required functions before DttR treats it as a component. Optional exports are loaded only when present.

The SDK macros in `dttr_components.h` declare these exports with the right names. Prefer them unless you have a reason to write the exports by hand.

## Required exports

### `dttr_component_init`

```c
bool dttr_component_init(const DTTR_ComponentContext *ctx);
```

Called once after DttR loads the component DLL, before any optional callbacks are used.

Return `true` to keep the component loaded. Return `false` to reject the host, report initialization failure, or disable the component for this run. DttR unloads the DLL when init fails.

Macro form:

```c
DTTR_COMPONENT_INIT {
    return true;
}
```

The macro rejects hosts whose `ctx->m_api_version` is older than `DTTR_COMPONENT_API_VERSION` before it enters the component body.

### `dttr_component_cleanup`

```c
void dttr_component_cleanup(void);
```

Called before DttR unloads an initialized component. Release hooks, allocations, and other component-owned state here.

Macro form:

```c
DTTR_COMPONENT_CLEANUP {
}
```

## Optional exports

### `dttr_component_info`

```c
const DTTR_ComponentInfo *dttr_component_info(void);
```

Returns component metadata for logging. DttR calls this before initialization and logs the name, version, author, and DLL filename when the function returns a non-`NULL` pointer.

The returned pointer must stay valid for the lifetime of the DLL. The SDK macro creates static storage for it:

```c
DTTR_COMPONENT_INFO("My Component", "1.0.0", "Author")
```

`DTTR_ComponentInfo` contains:

```c
typedef struct {
    const char *m_name;
    const char *m_version;
    const char *m_author;
} DTTR_ComponentInfo;
```

### Load lifecycle

| Export | Signature | Called |
| --- | --- | --- |
| `dttr_component_late_init` | `void dttr_component_late_init(void);` | After DttR and game late initialization finish during startup. |
| `dttr_component_before_unload` | `void dttr_component_before_unload(void);` | Immediately before DttR detaches component-owned hooks and calls cleanup. |

Macro form:

```c
DTTR_COMPONENT_LATE_INIT {
}

DTTR_COMPONENT_BEFORE_UNLOAD {
}
```

### `dttr_component_tick`

```c
void dttr_component_tick(void);
```

Called once per host-loop tick after DttR advances or blocks the game frame. DttR also checks for hot-reloaded component DLLs before running tick callbacks.

Macro form:

```c
DTTR_COMPONENT_TICK {
}
```

### Event and input lifecycle

| Export | Signature | Called |
| --- | --- | --- |
| `dttr_component_before_event` | `bool dttr_component_before_event(const SDL_Event *event);` | Before ImGui, component event callbacks, and built-in DttR event handling. Return `true` to consume the event immediately. |
| `dttr_component_event` | `bool dttr_component_event(const SDL_Event *event);` | After ImGui event processing and before built-in movie, input, audio, fullscreen, quit, and resize handling. Return `true` to consume the event. |
| `dttr_component_after_event` | `void dttr_component_after_event(const SDL_Event *event, bool consumed);` | After DttR finishes event handling. `consumed` is the final event-consumed state. |
| `dttr_component_input_mode_changed` | `void dttr_component_input_mode_changed(const DTTR_InputContext *ctx);` | When DttR publishes overlay visibility and game-input mode. |

Macro form:

```c
DTTR_COMPONENT_BEFORE_EVENT {
    return false;
}

DTTR_COMPONENT_EVENT {
    return false;
}

DTTR_COMPONENT_AFTER_EVENT {
}

DTTR_COMPONENT_INPUT_MODE_CHANGED {
}
```

### Frame lifecycle

| Export | Signature | Called |
| --- | --- | --- |
| `dttr_component_frame_begin` | `void dttr_component_frame_begin(const DTTR_FrameContext *ctx);` | At the start of a graphics frame. |
| `dttr_component_before_game_frame` | `void dttr_component_before_game_frame(const DTTR_FrameContext *ctx);` | Before the game renders into the game target. |
| `dttr_component_after_game_frame` | `void dttr_component_after_game_frame(const DTTR_FrameContext *ctx);` | After game-resolution rendering finishes. |
| `dttr_component_before_present` | `void dttr_component_before_present(const DTTR_PresentContext *ctx);` | Before DttR submits or presents the window frame. |
| `dttr_component_after_present` | `void dttr_component_after_present(const DTTR_PresentContext *ctx);` | After DttR submits or presents the window frame. |
| `dttr_component_frame_end` | `void dttr_component_frame_end(const DTTR_FrameContext *ctx);` | At the end of a graphics frame. |

`DTTR_FrameContext` contains:

```c
typedef struct {
    uint64_t m_frame_index;
    uint32_t m_window_w;
    uint32_t m_window_h;
    uint32_t m_game_x;
    uint32_t m_game_y;
    uint32_t m_game_w;
    uint32_t m_game_h;
    float m_scale;
} DTTR_FrameContext;
```

`DTTR_PresentContext` adds present-state flags:

```c
typedef struct {
    uint64_t m_frame_index;
    uint32_t m_window_w;
    uint32_t m_window_h;
    uint32_t m_game_x;
    uint32_t m_game_y;
    uint32_t m_game_w;
    uint32_t m_game_h;
    float m_scale;
    bool m_imgui_frame_active;
    bool m_overlay_rendered;
} DTTR_PresentContext;
```

Macro form:

```c
DTTR_COMPONENT_FRAME_BEGIN {
}

DTTR_COMPONENT_BEFORE_GAME_FRAME {
}

DTTR_COMPONENT_AFTER_GAME_FRAME {
}

DTTR_COMPONENT_BEFORE_PRESENT {
}

DTTR_COMPONENT_AFTER_PRESENT {
}

DTTR_COMPONENT_FRAME_END {
}
```

### ImGui lifecycle

| Export | Signature | Called |
| --- | --- | --- |
| `dttr_component_imgui_begin` | `void dttr_component_imgui_begin(const DTTR_RenderContext *ctx);` | After DttR starts a window-resolution ImGui frame and before component render callbacks. |
| `dttr_component_imgui_end` | `void dttr_component_imgui_end(const DTTR_RenderContext *ctx);` | After component render callbacks and before DttR renders ImGui draw data. |

Macro form:

```c
DTTR_COMPONENT_IMGUI_BEGIN {
}

DTTR_COMPONENT_IMGUI_END {
}
```

### Window and graphics lifecycle

| Export | Signature | Called |
| --- | --- | --- |
| `dttr_component_overlay_visible_changed` | `void dttr_component_overlay_visible_changed(bool visible);` | When DttR publishes an overlay visibility state. |
| `dttr_component_window_created` | `void dttr_component_window_created(const DTTR_WindowContext *ctx);` | After the SDL window is available to components. |
| `dttr_component_window_resized` | `void dttr_component_window_resized(const DTTR_WindowContext *ctx);` | After DttR applies a runtime window resize. |
| `dttr_component_window_destroying` | `void dttr_component_window_destroying(const DTTR_WindowContext *ctx);` | Before DttR destroys the SDL window. |
| `dttr_component_graphics_device_created` | `void dttr_component_graphics_device_created(const DTTR_GraphicsContext *ctx);` | After the graphics device/backend is available. |
| `dttr_component_graphics_device_lost` | `void dttr_component_graphics_device_lost(const DTTR_GraphicsContext *ctx);` | When DttR reports graphics device loss. |
| `dttr_component_graphics_device_restored` | `void dttr_component_graphics_device_restored(const DTTR_GraphicsContext *ctx);` | When DttR reports graphics device restoration. |
| `dttr_component_graphics_device_destroying` | `void dttr_component_graphics_device_destroying(const DTTR_GraphicsContext *ctx);` | Before DttR destroys the graphics device/backend. |

Macro form:

```c
DTTR_COMPONENT_WINDOW_CREATED {
}

DTTR_COMPONENT_GRAPHICS_DEVICE_CREATED {
}

DTTR_COMPONENT_GRAPHICS_DEVICE_DESTROYING {
}
```

Use the other lifecycle macros from [Component API](api.md#component-export-macros) when you need the matching export.

### `dttr_component_render_game`

```c
void dttr_component_render_game(const DTTR_RenderGameContext *ctx);
```

Called while DttR renders an ImGui frame at game resolution. Anything drawn here is letterboxed and scaled with the game image.

Use this for overlays that should stay inside the game's 4:3 render area.

```c
typedef struct {
    uint32_t m_width;
    uint32_t m_height;
    float m_scale;
} DTTR_RenderGameContext;
```

Macro form:

```c
DTTR_COMPONENT_RENDER_GAME {
}
```

### `dttr_component_render`

```c
void dttr_component_render(const DTTR_RenderContext *ctx);
```

Called while DttR renders an ImGui frame at full window resolution, above both the game image and the letterbox bars.

Use this for window-space overlays, tools, and UI.

```c
typedef struct {
    uint32_t m_window_w;
    uint32_t m_window_h;
    uint32_t m_game_x;
    uint32_t m_game_y;
    uint32_t m_game_w;
    uint32_t m_game_h;
    float m_scale;
} DTTR_RenderContext;
```

Macro form:

```c
DTTR_COMPONENT_RENDER {
}
```

### `dttr_component_should_advance_game_frame`

```c
bool dttr_component_should_advance_game_frame(void);
```

Called before DttR advances a game frame. If every component with this callback returns `true`, DttR calls the game's frame function and then notifies components through `dttr_component_game_frame_advanced`.

Return `false` to block the game frame while still letting DttR present overlays. If any component returns `false`, DttR skips the game frame for that host-loop iteration and then notifies components through `dttr_component_game_frame_blocked`.

Macro form:

```c
DTTR_COMPONENT_SHOULD_ADVANCE_GAME_FRAME {
    return true;
}
```

### `dttr_component_game_frame_advanced`

```c
void dttr_component_game_frame_advanced(void);
```

Called after DttR advances a game frame because no component blocked it.

Macro form:

```c
DTTR_COMPONENT_GAME_FRAME_ADVANCED {
}
```

### `dttr_component_game_frame_blocked`

```c
void dttr_component_game_frame_blocked(void);
```

Called after DttR presents overlays without advancing the game because at least one component returned `false` from `dttr_component_should_advance_game_frame`.

Macro form:

```c
DTTR_COMPONENT_GAME_FRAME_BLOCKED {
}
```

## Minimal component

```c
#include <dttr_components.h>

DTTR_COMPONENT_INFO("Minimal Component", "1.0.0", "Author")

DTTR_COMPONENT_INIT {
    DTTR_COMPONENT_LOG_INFO(ctx, "minimal component loaded");
    return true;
}

DTTR_COMPONENT_CLEANUP {
}
```
