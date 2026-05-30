# Drawing a GUI

DttR provides built-in ImGui support for mod overlay menus, including debug windows, mod options, small tools, and overlays that do not need to be drawn through the game renderer.

## Drawing UI in ImGui callbacks

Use these callbacks for ImGui widgets:

- `DTTR_MODS_IMGUI_BEGIN`: Add normal mod UI.
- `DTTR_MODS_IMGUI_END`: Add UI that should run near the end of the ImGui frame.

Most mods should use `DTTR_MODS_IMGUI_BEGIN`.

```c
DTTR_MODS_IMGUI_BEGIN {
    igBegin("My Mod", NULL, 0);
    igText("Hello from my mod");
    igEnd();
}
```

## Leaving the ImGui lifecycle to DttR

DttR owns the ImGui context, frame flow, SDL backend, and renderer backend. Submitting ImGui work outside a DttR ImGui callback can cause frame-order or backend-state bugs.

Calling these from a mod can break DttR's UI or another mod's UI:

- `igCreateContext` or `igDestroyContext`
- `igNewFrame` or `igRender`
- `ImGui_ImplSDL3_*` init or shutdown functions
- `ImGui_ImplOpenGL3_*` init or shutdown functions

## Storing UI state in your mod

Store checkbox values, window visibility, and other UI state in mod-owned variables.

```c
static bool show_window = true;

DTTR_MODS_IMGUI_BEGIN {
    if (!show_window) {
        return;
    }

    igBegin("My Mod", &show_window, 0);
    igText("Window state belongs to the mod");
    igEnd();
}
```

Storing pointers to short-lived ImGui data after the callback returns can create dangling-pointer bugs; copy any needed values instead.

## Reading SDL events from callbacks

For SDL event callbacks, include `SDL3/SDL.h` and read the callback data directly.

Forwarding events to ImGui backend APIs from a mod can create duplicate or out-of-order ImGui event handling; read the callback data directly instead.
