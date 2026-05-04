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

### `dttr_component_event`

```c
bool dttr_component_event(const SDL_Event *event);
```

Called for each SDL event after DttR forwards the event to ImGui and before DttR runs its built-in movie, input, audio, fullscreen, quit, and resize handling.

Return `true` to consume the event and stop DttR from handling it further. Return `false` to let DttR continue normal event processing.

Macro form:

```c
DTTR_COMPONENT_EVENT {
    return false;
}
```

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
