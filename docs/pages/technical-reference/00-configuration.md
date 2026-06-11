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
    "disabled_mods": []
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
    "deadzone_camera_rz": 700,
    "sensitivity_stick_x": 100,
    "sensitivity_stick_y": 100,
    "sensitivity_camera_rz": 100
  }
}
```

## Gamepad Analog Remap

`gamepad.analog_remap` is enabled by default. It routes left-stick X/Y through PS1-style analog scaling controlled by `deadzone_stick_x`, `deadzone_stick_y`, `sensitivity_stick_x`, and `sensitivity_stick_y`.

## Controller Vibration

Vibration follows the game's own vibration option and the global `gamepad.enabled` switch.

## Gamepad Buttons (`gamepad.buttons`)

`gamepad.buttons` is a JSON object. Each key is the physical input you press, and each value is the game action DttR sends. When the object is present, omitted inputs are unbound, so include every binding you want to keep.

This example binds three inputs and leaves the rest unbound:

```json
{
  "schema_major_version": 1,
  "gamepad": {
    "buttons": {
      "south": "joy_1",
      "east": "joy_2",
      "start": "joy_9"
    }
  }
}
```
