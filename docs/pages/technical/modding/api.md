# Component API

`dttr_components.h` is the C SDK header for component DLLs. Include it for the types, callbacks, and macros below. These are header declarations, not extra exports your DLL needs to define.

## Version and Export Helpers

| Name | Kind | Use |
| --- | --- | --- |
| `DTTR_COMPONENT_API_VERSION` | macro | Component API version expected by the SDK header. |
| `DTTR_EXPORT` | macro | Expands to `__declspec(dllexport)`. Used by the export macros. |

`DTTR_COMPONENT_INIT` checks `ctx->m_api_version` against `DTTR_COMPONENT_API_VERSION` before it runs your init body.

## Component Context

DttR passes `const DTTR_ComponentContext *ctx` into `dttr_component_init`:

| Field | Type | Use |
| --- | --- | --- |
| `m_api_version` | `uint32_t` | Host component API version. |
| `m_game_module` | `HMODULE` | Loaded game module. Use this for game sigscans. |
| `m_sidecar_module` | `HMODULE` | Loaded DttR sidecar module. |
| `m_window` | `SDL_Window *` | SDL window pointer. |
| `m_loader_dir` | `const char *` | Directory containing `dttr.exe`. |
| `m_exe_hash` | `const char *` | Hash string for the loaded game executable. |
| `m_config` | `const void *` | Opaque config pointer. Treat this as internal until the API grows typed accessors. |
| `m_api` | `const DTTR_ComponentAPI *` | General host API, currently logging. |
| `m_game_api` | `const DTTR_ComponentGameAPI *` | Game patching and hook helpers. |

Keep the pointer only while the component is loaded. If you store it globally, clear it in cleanup.

## Logging API

`ctx->m_api` exposes the raw logging callbacks:

| Field | Type | Use |
| --- | --- | --- |
| `m_log` | `DTTR_LogFn` | Log directly at a level. |
| `m_log_is_enabled` | `DTTR_LogIsEnabledFn` | Check whether a level is enabled. |
| `m_log_unchecked` | `DTTR_LogFn` | Log without checking the level first. |

Most components should use the macros instead:

| Macro | Level |
| --- | --- |
| `DTTR_COMPONENT_LOG_TRACE(ctx, ...)` | trace |
| `DTTR_COMPONENT_LOG_DEBUG(ctx, ...)` | debug |
| `DTTR_COMPONENT_LOG_INFO(ctx, ...)` | info |
| `DTTR_COMPONENT_LOG_WARN(ctx, ...)` | warn |
| `DTTR_COMPONENT_LOG_ERROR(ctx, ...)` | error |
| `DTTR_COMPONENT_LOG_FATAL(ctx, ...)` | fatal |

The numeric level macros are also exposed as `DTTR_COMPONENT_LOG_LVL_TRACE` through `DTTR_COMPONENT_LOG_LVL_FATAL`.

## Game Patching API

`ctx->m_game_api` exposes the low-level patching helpers:

| Field | Type | Use |
| --- | --- | --- |
| `m_sigscan` | `DTTR_SigscanFn` | Search a module for a byte signature and mask. |
| `m_hook_function` | `DTTR_HookFunctionFn` | Install a function hook and optionally return the original trampoline. |
| `m_hook_pointer` | `DTTR_HookPointerFn` | Replace an IAT or function-pointer slot and capture the original value. |
| `m_patch_bytes` | `DTTR_PatchBytesFn` | Patch bytes at an address. |
| `m_unhook` | `DTTR_UnhookFn` | Remove a hook or patch represented by `DTTR_Hook`. |

`DTTR_Hook` is an opaque handle. Store it and pass it back to `m_unhook`. Do not inspect it.

## Hook Storage and Install Macros

The header also includes macros for the common hook storage and install patterns:

| Macro | Use |
| --- | --- |
| `DTTR_STORAGE(type, name)` | Declare storage, or define it when `DTTR_INTEROP_IMPLEMENT` is set. |
| `DTTR_HOOK(name)` | Track `name_site` and `name_handle`. |
| `DTTR_TRAMPOLINE_HOOK(name)` | Track `name_site`, `name_handle`, and `name_trampoline`. |
| `DTTR_FUNC(name, cc, ret, params, args)` | Track a resolved function address and create a typed inline wrapper. |
| `DTTR_VAR(name, type)` | Track a resolved variable address and create `*_ptr`, `*_get`, and `*_set` helpers. |
| `DTTR_UNINSTALL(name, ctx)` | Unhook and clear a `DTTR_HOOK`. |
| `DTTR_TRAMPOLINE_UNINSTALL(name, ctx)` | Unhook and clear a `DTTR_TRAMPOLINE_HOOK`. |

Install and resolve macros:

| Macro | Use |
| --- | --- |
| `DTTR_INSTALL_JMP(name, ctx, sig, mask)` | Sigscan and patch a 5-byte `E9` jump to `name_callback`. Logs an error if not found. |
| `DTTR_INSTALL_JMP_OPTIONAL(name, ctx, sig, mask)` | Optional version of `DTTR_INSTALL_JMP`; no error if not found. |
| `DTTR_INSTALL_TRAMPOLINE(name, ctx, sig, mask, prologue)` | Sigscan, install a trampoline hook, and store `name_trampoline`. |
| `DTTR_INSTALL_TRAMPOLINE_AUTO(name, ctx, sig, mask)` | Trampoline install with automatic prologue sizing. |
| `DTTR_INSTALL_BYTES(name, ctx, sig, mask, offset, bytes, size)` | Sigscan and patch bytes at `match + offset`. Logs an error if not found. |
| `DTTR_INSTALL_BYTES_OPTIONAL(name, ctx, sig, mask, offset, bytes, size)` | Optional byte patch; no error if not found. |
| `DTTR_INSTALL_POINTER_AT(name, ctx, site, new_value)` | Install an IAT hook at a known address. |
| `DTTR_INSTALL_POINTER(name, ctx, sig, mask, site_expr)` | Sigscan, compute an IAT site from `match_`, and install `name_callback`. |
| `DTTR_RESOLVE(name, ctx, sig, mask, expr)` | Sigscan and resolve a `DTTR_FUNC` or `DTTR_VAR` address from `match`. |
| `DTTR_E8_TARGET(p)` | Resolve an `E8` relative call target. |
| `DTTR_FF25_ADDR(p)` | Read the target from an `FF 25` import thunk. |

!!! note

    Hook helpers assume the signatures and patch sizes match the loaded game executable. Check `ctx->m_exe_hash` when a hook only supports one executable, or keep the signatures strict enough.

## Metadata and Callback Context Types

| Type | Use |
| --- | --- |
| `DTTR_ComponentInfo` | Metadata returned by `dttr_component_info`. |
| `DTTR_FrameContext` | Per-frame index, window size, game viewport, and scale. |
| `DTTR_PresentContext` | Present-time frame context. Also includes `m_imgui_frame_active` and `m_overlay_rendered`. |
| `DTTR_WindowContext` | Window pointer, native `HWND`, and window size. |
| `DTTR_GraphicsBackend` | Backend enum: unknown, SDL GPU, or OpenGL. |
| `DTTR_GraphicsContext` | Window, `HWND`, backend, driver name, and render-target size. |
| `DTTR_InputContext` | Overlay visibility and game-input enabled state. |
| `DTTR_RenderGameContext` | Game-resolution render callback context. Contains `m_width`, `m_height`, and `m_scale`. |
| `DTTR_RenderContext` | Window-resolution render callback context. Contains window size, game viewport, and scale. |

## Callback Typedefs

The header exposes typedefs for every component callback DttR can load:

| Typedef | Function shape |
| --- | --- |
| `DTTR_ComponentInitFn` | `bool (*)(const DTTR_ComponentContext *ctx)` |
| `DTTR_ComponentCleanupFn` | `void (*)(void)` |
| `DTTR_ComponentTickFn` | `void (*)(void)` |
| `DTTR_ComponentEventFn` | `bool (*)(const SDL_Event *event)` |
| `DTTR_ComponentInfoFn` | `const DTTR_ComponentInfo *(*)(void)` |
| `DTTR_ComponentLateInitFn` | `void (*)(void)` |
| `DTTR_ComponentBeforeUnloadFn` | `void (*)(void)` |
| `DTTR_ComponentRenderGameFn` | `void (*)(const DTTR_RenderGameContext *ctx)` |
| `DTTR_ComponentRenderFn` | `void (*)(const DTTR_RenderContext *ctx)` |
| `DTTR_ComponentFrameBeginFn` | `void (*)(const DTTR_FrameContext *ctx)` |
| `DTTR_ComponentBeforeGameFrameFn` | `void (*)(const DTTR_FrameContext *ctx)` |
| `DTTR_ComponentAfterGameFrameFn` | `void (*)(const DTTR_FrameContext *ctx)` |
| `DTTR_ComponentBeforePresentFn` | `void (*)(const DTTR_PresentContext *ctx)` |
| `DTTR_ComponentAfterPresentFn` | `void (*)(const DTTR_PresentContext *ctx)` |
| `DTTR_ComponentFrameEndFn` | `void (*)(const DTTR_FrameContext *ctx)` |
| `DTTR_ComponentImguiBeginFn` | `void (*)(const DTTR_RenderContext *ctx)` |
| `DTTR_ComponentImguiEndFn` | `void (*)(const DTTR_RenderContext *ctx)` |
| `DTTR_ComponentOverlayVisibleChangedFn` | `void (*)(bool visible)` |
| `DTTR_ComponentWindowCreatedFn` | `void (*)(const DTTR_WindowContext *ctx)` |
| `DTTR_ComponentWindowResizedFn` | `void (*)(const DTTR_WindowContext *ctx)` |
| `DTTR_ComponentWindowDestroyingFn` | `void (*)(const DTTR_WindowContext *ctx)` |
| `DTTR_ComponentGraphicsDeviceCreatedFn` | `void (*)(const DTTR_GraphicsContext *ctx)` |
| `DTTR_ComponentGraphicsDeviceLostFn` | `void (*)(const DTTR_GraphicsContext *ctx)` |
| `DTTR_ComponentGraphicsDeviceRestoredFn` | `void (*)(const DTTR_GraphicsContext *ctx)` |
| `DTTR_ComponentGraphicsDeviceDestroyingFn` | `void (*)(const DTTR_GraphicsContext *ctx)` |
| `DTTR_ComponentBeforeEventFn` | `bool (*)(const SDL_Event *event)` |
| `DTTR_ComponentAfterEventFn` | `void (*)(const SDL_Event *event, bool consumed)` |
| `DTTR_ComponentInputModeChangedFn` | `void (*)(const DTTR_InputContext *ctx)` |
| `DTTR_ComponentShouldAdvanceGameFrameFn` | `bool (*)(void)` |
| `DTTR_ComponentGameFrameAdvancedFn` | `void (*)(void)` |
| `DTTR_ComponentGameFrameBlockedFn` | `void (*)(void)` |

## Component Export Macros

Use these instead of writing the exported function declarations by hand:

| Macro | Defines |
| --- | --- |
| `DTTR_COMPONENT_INFO(name, version, author)` | `dttr_component_info` with static metadata storage. |
| `DTTR_COMPONENT_INIT` | `dttr_component_init` with API-version check. |
| `DTTR_COMPONENT_CLEANUP` | `dttr_component_cleanup`. |
| `DTTR_COMPONENT_TICK` | `dttr_component_tick`. |
| `DTTR_COMPONENT_LATE_INIT` | `dttr_component_late_init`. |
| `DTTR_COMPONENT_BEFORE_UNLOAD` | `dttr_component_before_unload`. |
| `DTTR_COMPONENT_FRAME_BEGIN` | `dttr_component_frame_begin`. |
| `DTTR_COMPONENT_BEFORE_GAME_FRAME` | `dttr_component_before_game_frame`. |
| `DTTR_COMPONENT_AFTER_GAME_FRAME` | `dttr_component_after_game_frame`. |
| `DTTR_COMPONENT_BEFORE_PRESENT` | `dttr_component_before_present`. |
| `DTTR_COMPONENT_AFTER_PRESENT` | `dttr_component_after_present`. |
| `DTTR_COMPONENT_FRAME_END` | `dttr_component_frame_end`. |
| `DTTR_COMPONENT_IMGUI_BEGIN` | `dttr_component_imgui_begin`. |
| `DTTR_COMPONENT_IMGUI_END` | `dttr_component_imgui_end`. |
| `DTTR_COMPONENT_OVERLAY_VISIBLE_CHANGED` | `dttr_component_overlay_visible_changed`. |
| `DTTR_COMPONENT_WINDOW_CREATED` | `dttr_component_window_created`. |
| `DTTR_COMPONENT_WINDOW_RESIZED` | `dttr_component_window_resized`. |
| `DTTR_COMPONENT_WINDOW_DESTROYING` | `dttr_component_window_destroying`. |
| `DTTR_COMPONENT_GRAPHICS_DEVICE_CREATED` | `dttr_component_graphics_device_created`. |
| `DTTR_COMPONENT_GRAPHICS_DEVICE_LOST` | `dttr_component_graphics_device_lost`. |
| `DTTR_COMPONENT_GRAPHICS_DEVICE_RESTORED` | `dttr_component_graphics_device_restored`. |
| `DTTR_COMPONENT_GRAPHICS_DEVICE_DESTROYING` | `dttr_component_graphics_device_destroying`. |
| `DTTR_COMPONENT_BEFORE_EVENT` | `dttr_component_before_event`. |
| `DTTR_COMPONENT_AFTER_EVENT` | `dttr_component_after_event`. |
| `DTTR_COMPONENT_INPUT_MODE_CHANGED` | `dttr_component_input_mode_changed`. |
| `DTTR_COMPONENT_EVENT` | `dttr_component_event`. |
| `DTTR_COMPONENT_RENDER_GAME` | `dttr_component_render_game`. |
| `DTTR_COMPONENT_RENDER` | `dttr_component_render`. |
| `DTTR_COMPONENT_SHOULD_ADVANCE_GAME_FRAME` | `dttr_component_should_advance_game_frame`. |
| `DTTR_COMPONENT_GAME_FRAME_ADVANCED` | `dttr_component_game_frame_advanced`. |
| `DTTR_COMPONENT_GAME_FRAME_BLOCKED` | `dttr_component_game_frame_blocked`. |
