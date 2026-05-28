# Drawing a GUI

DttR provides built-in ImGui support for mods, allowing them to more easily implement overlay menus for a variety of purposes, including debug windows, mod options, small tools, and overlays that do not need to be drawn directly through the game.

## Use the ImGui callbacks

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

## Only submit widgets

DttR owns the ImGui context, frame flow, SDL backend, and renderer backend. A mod should only submit widgets while DttR is inside an ImGui callback.

In short, do not call these from a mod:

- `igCreateContext` or `igDestroyContext`
- `igNewFrame` or `igRender`
- `ImGui_ImplSDL3_*` init or shutdown functions
- `ImGui_ImplOpenGL3_*` init or shutdown functions

If your mod calls those functions, it can break DttR's UI or another mod's UI.

## Keep UI state in your mod

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

Do not store pointers to short-lived ImGui data after the callback returns.

## Read SDL events directly

For SDL event callbacks, include `SDL3/SDL.h` and read the callback data directly.

Do not forward events to ImGui backend APIs yourself. DttR already handles ImGui backend event flow.
