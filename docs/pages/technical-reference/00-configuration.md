---
title: "Configuration Reference"
description: "Reference for dttr.json settings, including schema version, game paths, graphics, audio, controller, logging, crash dumps, and modding options."
seo_type: "article"
---

# Configuration

DttR stores settings in `dttr.json` next to `dttr.exe`. Invalid JSON or a missing `schema_major_version` causes a configuration error.

If `dttr.json` is missing or empty, DttR uses the built-in defaults, creating the file only when it is missing.

Pass a config file to `dttr.exe` to use multiple persistent configurations:

```sh
dttr.exe custom_dttr.json
```

Set string fields to `null` to clear them. DttR treats unknown or mistyped scalar keys as configuration errors.

## Example

```json
{
  "schema_major_version": 1,
  "log_level": "info",
  "minidump_type": "normal",
  "show_crash_popup": true,
  "log_file_path": "dttr.log",
  "pcdogs_path": "",
  "saves_path": "saves",
  "skip_intro_movies": false,
  "graphics": {
    "scaling_fit": "letterbox",
    "scaling_method": "logical",
    "graphics_api": "auto",
    "present_scaling_algorithm": "linear",
    "window_width": 640,
    "window_height": 480,
    "msaa_samples": 2,
    "texture_upload_sync": false,
    "generate_texture_mipmaps": true,
    "vertex_precision": "native",
    "sprite_smooth": true,
    "fullscreen": false
  },
  "audio": {
    "mss_sample_gain": 1.0,
    "mss_sample_preemphasis": 0.0
  },
  "modding": {
    "hot_reload": false,
    "disabled_mods": [],
    "mod_configs": {}
  },
  "gamepad": {
    "enabled": true,
    "analog_remap": true,
    "index": 0,
    "axis_stick_x": "axis_left_x",
    "axis_stick_y": "axis_left_y",
    "axis_camera_rz": "axis_right_x",
    "deadzone_stick_x": 333,
    "deadzone_stick_y": 333,
    "deadzone_camera_rz": 600,
    "sensitivity_stick_x": 100,
    "sensitivity_stick_y": 100,
    "sensitivity_camera_rz": 100
  }
}
```

## Mod Configs (`modding.mod_configs`)

Mod settings are stored by stable SDK `mod_id`. Each entry has a `schema_version` and a `values` object. Values may be booleans, integers, floats, or strings; enums are stored as strings. The settings come from mods that export `DTTR_Mod_Config`.

```json
{
  "modding": {
    "mod_configs": {
      "example.author.my_mod": {
        "schema_version": 1,
        "values": {
          "enabled": true,
          "mode": "fast"
        }
      }
    }
  }
}
```

## Gamepad Analog Remap

`gamepad.analog_remap` routes left-stick X/Y through PS1-style analog scaling and is enabled by default.

## Control Bindings (`controls.special_bindings`)

`controls.special_bindings` stores extra bindings for inputs that the in-game controls menu does not expose.

Supported entries:

| Key | Native/default source | Notes |
| --- | --- | --- |
| `start_pause` | Escape/gamepad start paths; mask `0x8000` | Extra pause/start mask. |
| `menu_confirm` | Controls-menu Enter path | Controls-menu prompt only; does not spoof global Enter. |
| `menu_cancel` | Controls-menu Escape path | Controls-menu remap abort only; does not spoof global Escape. |

Movement/confirm/back stay in the in-game Controls menu. `special_bindings` only covers controls that menu cannot edit. Missing keys keep the game default and are omitted when saving.

```json
{
  "schema_major_version": 1,
  "controls": {
    "special_bindings": {
      "menu_confirm": 298
    }
  }
}
```
