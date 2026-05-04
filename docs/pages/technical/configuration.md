# Configuration

DttR stores its settings in `dttr.json`. The file must be strict JSON. If it is missing, DttR writes a fresh one from the built-in defaults.

Start DttR with another config file when you want a separate setup:

```sh
dttr.exe path/to/my_config.json
```

## Example

```json
{
  "schema_major_version": 1,
  "graphics": {
    "graphics_api": "auto",
    "scaling_fit": "letterbox",
    "scaling_method": "logical",
    "window_width": 640,
    "window_height": 480,
    "fullscreen": false
  },
  "audio": {},
  "gamepad": {
    "enabled": true,
    "index": 0
  }
}
```

## Top-level keys

| Key | Type | Values | Default | Description |
| --- | --- | --- | --- | --- |
| `schema_major_version` | Integer | `1` | `1` | Config schema version. |
| `log_level` | String | `trace`, `debug`, `info`, `warn`, `error`, `fatal` | `debug` in debug builds; `info` in release builds | Minimum log level. |
| `minidump_type` | String | `normal`, `detailed` | `detailed` in debug builds; `normal` in release builds | Crash minidump detail level. |
| `log_file_path` | String | Path | `dttr.log` | Log file path. |
| `pcdogs_path` | String | Path | Empty | Game executable or game data path. |
| `saves_path` | String | Path | `saves` | Save directory. |

## `graphics`

Graphics settings belong under `graphics`.

| Key | Type | Values | Default | Description |
| --- | --- | --- | --- | --- |
| `graphics_api` | String | `auto`, `vulkan`, `direct3d12`, `opengl` | `auto` | Renderer backend. |
| `scaling_fit` | String | `letterbox`, `stretch`, `integer` | `letterbox` | How the game image fits the output. |
| `scaling_method` | String | `present`, `logical` | `logical` | Where scaling is applied. |
| `present_scaling_algorithm` | String | `nearest`, `linear` | `nearest` | Sampling used by present scaling. |
| `window_width` | Integer | Positive integer | `640` | Initial window width. |
| `window_height` | Integer | Positive integer | `480` | Initial window height. |
| `msaa_samples` | Integer | Sample count | `2` | Multisample count. |
| `texture_upload_sync` | Boolean | `true`, `false` | `false` | Synchronize texture uploads. |
| `generate_texture_mipmaps` | Boolean | `true`, `false` | `true` | Generate texture mipmaps. |
| `vertex_precision` | String | `native`, `subpixel` | `native` | Vertex positioning precision. |
| `sprite_smooth` | Boolean | `true`, `false` | `true` | Smooth sprite sampling. |
| `fullscreen` | Boolean | `true`, `false` | `false` | Start fullscreen. |

Alias: `d3d12`.

## `audio`

Audio settings belong under `audio`.

| Key | Type | Values | Default | Description |
| --- | --- | --- | --- | --- |
| `mss_sample_gain` | Number | Floating-point number | `1.0` | Sample gain. |
| `mss_sample_preemphasis` | Number | Floating-point number | `0.0` | Sample preemphasis. |

## `gamepad`

Gamepad settings belong under `gamepad`.

| Key | Type | Values | Default | Description |
| --- | --- | --- | --- | --- |
| `enabled` | Boolean | `true`, `false` | `true` | Enable SDL gamepad input. |
| `index` | Integer | SDL gamepad index | `0` | SDL gamepad index. |
| `stick_x` | String | [Axis token](#axis-tokens) | `axis_left_x` | Horizontal movement axis. |
| `stick_y` | String | [Axis token](#axis-tokens) | `axis_left_y` | Vertical movement axis. |
| `camera_rz` | String | [Axis token](#axis-tokens) | `axis_right_x` | Camera axis. |
| `stick_x_deadzone` | Integer | SDL axis deadzone | `700` | Horizontal movement deadzone. |
| `stick_y_deadzone` | Integer | SDL axis deadzone | `700` | Vertical movement deadzone. |
| `camera_rz_deadzone` | Integer | SDL axis deadzone | `700` | Camera axis deadzone. |

### Axis tokens

| Token | Description |
| --- | --- |
| `none` | No axis. |
| `axis_left_x` | Left stick X axis. |
| `axis_left_y` | Left stick Y axis. |
| `axis_right_x` | Right stick X axis. |
| `axis_right_y` | Right stick Y axis. |
| `axis_left_trigger` | Left trigger axis. |
| `axis_right_trigger` | Right trigger axis. |

## `gamepad.buttons`

`gamepad.buttons` maps [source tokens](#button-source-tokens) to [action tokens](#button-action-tokens).

### Button source tokens

| Token | Description |
| --- | --- |
| `south`, `east`, `west`, `north` | Face buttons. |
| `back`, `guide`, `start` | Menu and system buttons. |
| `left_stick_click`, `right_stick_click` | Stick click buttons. |
| `left_shoulder`, `right_shoulder` | Shoulder buttons. |
| `dpad_up`, `dpad_down`, `dpad_left`, `dpad_right` | Directional pad buttons. |
| `left_trigger`, `right_trigger` | Trigger buttons. |

### Button action tokens

| Token | Description |
| --- | --- |
| `none` | No action. |
| `up`, `down`, `left`, `right` | Directional actions. |
| `pov_up`, `pov_down` | POV actions. |
| `joy_1` through `joy_13` | Original game joystick buttons. |

## `modding`

Modding settings belong under `modding`.

| Key | Type | Values | Default | Description |
| --- | --- | --- | --- | --- |
| `hot_reload` | Boolean | `true`, `false` | `false` | Reload components while DttR runs. |
| `disabled_components` | Array of strings | Component DLL filenames | Empty | Component DLL filenames to skip. |
