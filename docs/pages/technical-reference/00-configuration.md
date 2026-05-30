# Configuration

DttR stores settings in `dttr.json` next to `dttr.exe`. Invalid JSON or a missing `schema_major_version` will cause a configuration error.

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
  "show_crash_stack_trace": true,
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
    "index": 0,
    "axis_stick_x": "axis_left_x",
    "axis_stick_y": "axis_left_y",
    "axis_camera_rz": "axis_right_x",
    "deadzone_stick_x": 700,
    "deadzone_stick_y": 700,
    "deadzone_camera_rz": 700
  }
}
```

## Top-Level Keys

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `schema_major_version` | Integer | `1` | Config schema version. Values other than `1` will cause a configuration error. |
| `log_level` | String | `debug` in debug builds, `info` in release builds | Minimum log level: `trace`, `debug`, `info`, `warn`, `error`, or `fatal`. |
| `minidump_type` | String | `detailed` in debug builds, `normal` in release builds | Crash minidump detail level: `normal` or `detailed`. |
| `show_crash_stack_trace` | Boolean | `true` | Show stack traces in crash popups. Full stack traces are still written to `dttr.log` when this is `false`. |
| `log_file_path` | String | `dttr.log` | Relative paths resolve from the DttR directory. |
| `pcdogs_path` | String | Empty | Extracted/installed game directory or ISO path. |
| `saves_path` | String | `saves` | Save-redirection root. Relative paths resolve from the DttR directory. Empty or `null` disables save redirection. |
| `skip_intro_movies` | Boolean | `false` | Skip the intro movies at launch. |

## Graphics (`graphics`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `graphics_api` | String | `auto` | Renderer backend: `auto`, `vulkan`, `direct3d12`, or `opengl`. `d3d12` is accepted as an alias for `direct3d12`. |
| `scaling_fit` | String | `letterbox` | How the game image fits the output: `letterbox`, `stretch`, or `integer`. |
| `scaling_method` | String | `logical` | Where scaling is applied: `present` or `logical`. |
| `present_scaling_algorithm` | String | `linear` | Sampling used when `scaling_method` is `present`: `nearest` or `linear`. |
| `window_width` | Integer | `640` | Initial window width in pixels. Numbers below 64 fall back to 640 at runtime. |
| `window_height` | Integer | `480` | Initial window height in pixels. Numbers below 64 fall back to 480 at runtime. |
| `msaa_samples` | Integer | `2` | Multisample count. `1` disables MSAA. SDL GPU backends support `1`, `2`, `4`, and `8`. OpenGL clamps to device support. Unsupported SDL GPU counts disable MSAA. |
| `texture_upload_sync` | Boolean | `false` | Synchronize texture uploads. |
| `generate_texture_mipmaps` | Boolean | `true` | Generate texture mipmaps. |
| `vertex_precision` | String | `native` | Vertex positioning precision: `native` or `subpixel`. |
| `sprite_smooth` | Boolean | `true` | Smooth sprite sampling. |
| `fullscreen` | Boolean | `false` | Start fullscreen. |

## Audio (`audio`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `mss_sample_gain` | Number | `1.0` | Floating-point sample gain. |
| `mss_sample_preemphasis` | Number | `0.0` | Floating-point sample preemphasis. |

## Gamepad (`gamepad`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `enabled` | Boolean | `true` | Enable SDL gamepad input. |
| `index` | Integer | `0` | SDL gamepad index. |
| `axis_stick_x` | String | `axis_left_x` | Horizontal movement axis. Use an [axis token](#axis-tokens). |
| `axis_stick_y` | String | `axis_left_y` | Vertical movement axis. Use an [axis token](#axis-tokens). |
| `axis_camera_rz` | String | `axis_right_x` | Camera axis. Use an [axis token](#axis-tokens). |
| `deadzone_stick_x` | Integer | `700` | Horizontal movement deadzone in SDL axis units. |
| `deadzone_stick_y` | Integer | `700` | Vertical movement deadzone in SDL axis units. |
| `deadzone_camera_rz` | Integer | `700` | Camera axis deadzone in SDL axis units. |

### Axis Tokens

| Token | Description |
| --- | --- |
| `none` | No axis. |
| `axis_left_x` | Left stick X axis. |
| `axis_left_y` | Left stick Y axis. |
| `axis_right_x` | Right stick X axis. |
| `axis_right_y` | Right stick Y axis. |
| `axis_left_trigger` | Left trigger axis. |
| `axis_right_trigger` | Right trigger axis. |

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

### Button Source Tokens

| Token | Description |
| --- | --- |
| `south`, `east`, `west`, `north` | Face buttons. |
| `back`, `guide`, `start` | Menu and system buttons. |
| `left_stick_click`, `right_stick_click` | Stick click buttons. |
| `left_shoulder`, `right_shoulder` | Shoulder buttons. |
| `dpad_up`, `dpad_down`, `dpad_left`, `dpad_right` | Directional pad buttons. |
| `misc1` through `misc6` | SDL miscellaneous buttons, when the controller exposes them. |
| `right_paddle1`, `right_paddle2`, `left_paddle1`, `left_paddle2` | Paddle buttons, when the controller exposes them. |
| `touchpad` | Touchpad button. |
| `left_trigger`, `right_trigger` | Trigger buttons. |

### Button Action Tokens

| Token | Description |
| --- | --- |
| `none` | No action. |
| `up`, `down`, `left`, `right` | Directional actions. |
| `pov_up`, `pov_down` | POV actions. |
| `joy_1` through `joy_13` | Original game joystick buttons. |

## Modding (`modding`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `hot_reload` | Boolean | `false` | Reload mods while DttR runs. |
| `disabled_mods` | Array of strings | Empty | Up to 32 mod DLL filenames to skip. Names are matched case-insensitively. |
