---
title: "Defining Custom Settings"
description: "Expose declarative mod settings in the config GUI and read saved values from your mod."
seo_type: "article"
---

# Defining Custom Settings

Mods expose pre-runtime settings in the config GUI by exporting `DTTR_Mod_Config`.

The config GUI shows them under **Modding > Mod Configs** and writes values to `dttr.json`.

## Declaring Settings

Mod settings need a unique `mod_id` so they stay isolated and persist across versions.

```c
#include <dttr_sdk.h>

static const DTTR_Mods_ConfigChoice corner_choices[] = {
    {"top_left", "Top left", "Draw in the top-left corner."},
    {"top_right", "Top right", "Draw in the top-right corner."},
    {"bottom_left", "Bottom left", "Draw in the bottom-left corner."},
    {"bottom_right", "Bottom right", "Draw in the bottom-right corner."},
};

static const DTTR_Mods_ConfigField config_fields[] = {
    {
        .struct_size = sizeof(DTTR_Mods_ConfigField),
        .id = "move_speed",
        .label = "Movement speed",
        .tooltip = "Speed multiplier for the puppy.",
        .type = DTTR_MODS_CONFIG_FIELD_FLOAT,
        .default_value = {.float_value = 1.0f},
        .float_min = 0.25f,
        .float_max = 4.0f,
    },
    {
        .struct_size = sizeof(DTTR_Mods_ConfigField),
        .id = "starting_lives",
        .label = "Starting lives",
        .tooltip = "Lives the puppy starts each level with.",
        .type = DTTR_MODS_CONFIG_FIELD_INT,
        .default_value = {.int_value = 9},
        .int_min = 1,
        .int_max = 99,
    },
    {
        .struct_size = sizeof(DTTR_Mods_ConfigField),
        .id = "show_fps",
        .label = "Show FPS counter",
        .tooltip = "Draw a frame-rate counter while playing.",
        .type = DTTR_MODS_CONFIG_FIELD_BOOL,
        .default_value = {.bool_value = false},
    },
    {
        .struct_size = sizeof(DTTR_Mods_ConfigField),
        .id = "fps_corner",
        .label = "FPS counter position",
        .tooltip = "Screen corner for the frame-rate counter.",
        .type = DTTR_MODS_CONFIG_FIELD_ENUM,
        .default_value = {.string_value = "top_left"},
        .choices = corner_choices,
        .choice_count = DTTR_ARRAY_COUNT(corner_choices),
    },
    {
        .struct_size = sizeof(DTTR_Mods_ConfigField),
        .id = "screenshot_key",
        .label = "Screenshot key",
        .tooltip = "Press to save a screenshot.",
        .type = DTTR_MODS_CONFIG_FIELD_INPUT_BINDING,
        .default_value = {.string_value = "key:f5"},
    },
};

static const DTTR_Mods_ConfigSpec config_spec = {
    .struct_size = sizeof(DTTR_Mods_ConfigSpec),
    .schema_version = 1,
    .mod_id = "example.author.tweaks",
    .label = "Gameplay Tweaks",
    .fields = config_fields,
    .field_count = DTTR_ARRAY_COUNT(config_fields),
};

DTTR_MODS_CONFIG {
    return &config_spec;
}
```

Supported field types:

- `DTTR_MODS_CONFIG_FIELD_BOOL`
- `DTTR_MODS_CONFIG_FIELD_INT`
- `DTTR_MODS_CONFIG_FIELD_FLOAT`
- `DTTR_MODS_CONFIG_FIELD_STRING`
- `DTTR_MODS_CONFIG_FIELD_ENUM`
- `DTTR_MODS_CONFIG_FIELD_INPUT_BINDING`

Enums store their selected `choice.value` as a string.

## Input Bindings

An `INPUT_BINDING` field lets players bind one keyboard key, mouse button, or gamepad button in the config GUI. Values use `device:name`:

- `key:<name>`: A keyboard key, e.g. `key:f5`, `key:left_shift`, `key:space`
- `mouse:<name>`: `mouse:left`, `mouse:middle`, `mouse:right`, `mouse:x1`, `mouse:x2`
- `pad:<name>`: A gamepad button using SDL's names, e.g. `pad:a`, `pad:start`, `pad:dpup`

Defaults use `default_value.string_value`; empty or **Clear** means unbound.

Read it with `config_get_input_binding`, which fills a resolved `DTTR_Mods_ConfigInputBinding`:

```c
typedef struct {
    uint32_t struct_size;
    DTTR_Mods_BindingDevice device; // NONE, KEYBOARD, MOUSE, or GAMEPAD
    int code;
} DTTR_Mods_ConfigInputBinding;
```

`code` holds a raw SDL value you can compare straight against the matching event, so the device tells you which one to check:

| `device` | `code` Type | Compare against |
| --- | --- | --- |
| `DTTR_MODS_BINDING_KEYBOARD` | `SDL_Scancode` | `event->key.scancode` |
| `DTTR_MODS_BINDING_MOUSE` | `SDL_BUTTON_*` | `event->button.button` |
| `DTTR_MODS_BINDING_GAMEPAD` | `SDL_GamepadButton` | `event->gbutton.button` |
| `DTTR_MODS_BINDING_NONE` | N/A | N/A |

**NOTE: Esc cancels capture, so it can't be bound.**

## Reading Values at Runtime

To read a mod setting, call into `ctx->api` during init or a later callback. Config accessors return `DTTR_Result`; outputs are updated only on `DTTR_OK`, so failed reads leave your initialized defaults in place.

```c
static float move_speed = 1.0f;
static int starting_lives = 9;
static bool show_fps = false;
static char fps_corner[256] = "top_left";
static DTTR_Mods_ConfigInputBinding screenshot_key = {0};

DTTR_MODS_INIT {
    mod_ctx = ctx;

    // Accessors return function pointers; guard against NULL before calling.
    // A failed read leaves the variable at its initialized default, so the result is
    // safe to ignore here.
    DTTR_Mods_ConfigGetFloatFn get_float = DTTR_Mods_GetConfigFloatFn(ctx->api);
    DTTR_Mods_ConfigGetIntFn get_int = DTTR_Mods_GetConfigIntFn(ctx->api);
    DTTR_Mods_ConfigGetBoolFn get_bool = DTTR_Mods_GetConfigBoolFn(ctx->api);
    DTTR_Mods_ConfigGetStringFn get_string = DTTR_Mods_GetConfigStringFn(ctx->api);
    DTTR_Mods_ConfigGetInputBindingFn get_binding =
        DTTR_Mods_GetConfigInputBindingFn(ctx->api);

    const char *id = "example.author.tweaks";
    if (get_float) get_float(id, "move_speed", &move_speed);
    if (get_int) get_int(id, "starting_lives", &starting_lives);
    if (get_bool) get_bool(id, "show_fps", &show_fps);
    if (get_string) get_string(id, "fps_corner", fps_corner, sizeof(fps_corner));
    if (get_binding) get_binding(id, "screenshot_key", &screenshot_key);

    return true;
}

DTTR_MODS_EVENT {
    if (screenshot_key.device == DTTR_MODS_BINDING_KEYBOARD
        && event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat
        && event->key.scancode == screenshot_key.code) {
        save_screenshot();
    }

    return false;
}
```

## Persistence

DttR stores mod settings under `modding.mod_configs`:

```json
{
  "modding": {
    "mod_configs": {
      "example.author.tweaks": {
        "schema_version": 1,
        "values": {
          "move_speed": 1.0,
          "starting_lives": 9,
          "show_fps": false,
          "fps_corner": "top_left",
          "screenshot_key": "key:f5"
        }
      }
    }
  }
}
```
