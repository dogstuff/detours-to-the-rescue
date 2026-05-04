# Configuration

Run `dttr_config.exe` from the same folder as `dttr.exe` when you want to change settings without editing `dttr.json` by hand.

![DttR configuration window](assets/config-gui.png)

Save your changes before closing the configuration tool.

## General

### Game directory or ISO

Use **Game directory or ISO** when you need to switch discs, choose a different installed copy, or fix a path after moving files.

### Save folder

By default, DttR writes saves to `saves` next to `dttr.exe`.

Change it if:

- Windows cannot write to the DttR folder
- you want saves in a backed-up location
- you want separate save folders for testing or speedrunning

## Graphics

### Scaling method

Change this if the game looks blurry, stretched, or scaled in the wrong place.

- `logical` is the normal choice.
- `present` scales the final image instead. Use it mainly when comparing renderers.

### Scaling fit

This decides how the game image fits inside the window.

- `letterbox` keeps the correct aspect ratio and adds borders if needed.
- `stretch` fills the whole window, even if that distorts the image.
- `integer` scales in whole-number steps for sharper pixels.

Start with `letterbox` unless you are deliberately going for another look.

### Vertex precision

This decides how closely DttR follows the original game's geometry.

- `native` keeps the original-style vertex positioning.
- `subpixel` allows smoother polygon movement, but currently reveals seams in some models.

### Graphics API

Leave this on `auto` unless startup or rendering problems force you to pick a backend.

- `auto` lets DttR choose the best graphics API for the machine.
- `vulkan`, `direct3d12`, and `opengl` force a backend.

### Other video options

- `window_width` and `window_height` set the startup window size.
- `fullscreen` starts DttR fullscreen. You can also toggle fullscreen in game with ++f11++.
- `sprite_smooth` smooths scaled sprites. Turn it off for sharper pixels.
- `msaa_samples` smooths 3D edges at some performance cost.
- `generate_texture_mipmaps` can make scaled textures look smoother.

Leave texture upload synchronization alone unless you are already debugging a renderer issue.

## Audio

### Audio output

Leave SDL-backed audio enabled for normal play. It routes the game's old Miles Sound System calls through SDL, which is the tested path on modern Windows.

Turn it off only when you are comparing against the original Miles path while debugging audio. If that changes the problem, mention it in the bug report.

### Volume and sample tuning

- `mss_sample_gain` makes game audio louder or quieter. Use small changes; large values can clip.
- `mss_sample_preemphasis` changes how samples are filtered before playback. Leave it at the default unless you are deliberately tuning or comparing audio output.

## Gamepad

### Enable and pick a controller

Enable gamepad support, save, then start the game with the controller connected.

The controller index is SDL's gamepad number. Keep it at `0` for one controller. If DttR listens to the wrong controller, change the index, save, and restart the game.

### Sticks and axes

The default layout uses the left stick for movement and the right stick for the camera.

Change axis bindings when:

- movement is on the wrong stick
- the camera moves on the wrong axis
- you want a trigger or unused stick to do nothing

Set an axis to `none` to disable it.

### Deadzones

Deadzones ignore small stick movements near the center.

Raise a deadzone if a stick drifts while centered. Lower it if movement or camera control feels unresponsive near the center. Change values in small steps, save, then test in game.

### Buttons

Each button binding maps a physical controller button to one original game joystick action.

Change one binding at a time, then test in game. If a button does the wrong thing, set it back before changing the next one.

Set a button to `none` to leave it unused.

## Advanced editing

You can edit `dttr.json` directly, or start DttR with another config file:

```sh
dttr.exe path/to/my_config.json
```

See [Configuration (Technical)](technical/configuration.md) for the full JSON key reference.
