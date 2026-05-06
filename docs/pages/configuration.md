# Configuration

Run `dttr_config.exe` next to `dttr.exe` to change settings without editing `dttr.json` by hand.

![DttR configuration window](assets/config-gui.png)

Save before closing the configuration tool.

## General

### Game directory or ISO

Use **Game directory or ISO** to switch discs, choose a different installed copy, or fix a path after moving files.

### Save folder

DttR writes saves to `saves` next to `dttr.exe` by default.

Change it when:

- Windows cannot write to the DttR folder
- you want saves in a backed up location
- you want separate save folders for testing or speedrunning

### Intro movies

Enable **Skip intro movies** to skip the intro movies/videos.

## Graphics

### Scaling method

Use `logical` unless you are comparing renderers or debugging scaling.

- `logical` is the normal choice.
- `present` scales the final image instead.

### Scaling fit

- `letterbox` keeps the correct aspect ratio and adds borders if needed.
- `stretch` fills the whole window, even if that distorts the image.
- `integer` scales in whole-number steps for sharper pixels.

Start with `letterbox` unless you are deliberately going for another look.

### Vertex precision

- `native` keeps the original-style vertex positioning.
- `subpixel` allows smoother polygon movement, but it currently reveals seams in some models.

### Graphics API

Keep this on `auto` unless startup or rendering problems force you to pick a backend.

- `auto` lets DttR choose the best graphics API for the machine.
- `vulkan`, `direct3d12`, and `opengl` force a backend.

### Other video options

- `window_width` and `window_height` set the startup window size.
- `fullscreen` starts DttR fullscreen. You can also toggle fullscreen in game with ++f11++.
- `sprite_smooth` smooths scaled sprites. Turn it off for sharper pixels.
- `msaa_samples` smooths 3D edges at some performance cost.
- `generate_texture_mipmaps` can make scaled textures look smoother.

Leave texture upload synchronization at its default unless you are already debugging a renderer issue.

## Audio

### Audio output

Audio always routes the game's old Miles Sound System calls through SDL. This is the tested path on modern Windows.

### Volume and sample tuning

- `mss_sample_gain` makes game audio louder or quieter. Use small changes; large values can clip.
- `mss_sample_preemphasis` changes how samples are filtered before playback. Leave it at the default unless you are deliberately tuning or comparing audio output.

## Gamepad

### Enable and pick a controller

Enable gamepad support, save, then start the game with the controller connected.

The controller index is SDL's gamepad number. Keep it at `0` for one controller. If DttR listens to the wrong controller, change the index, save, and restart.

### Sticks and axes

The default layout uses the left stick for movement and the right stick for the camera.

Change axis bindings when:

- Movement is on the wrong stick
- The camera moves on the wrong axis
- You want a trigger or unused stick to do nothing

Set an axis to `none` to disable it.

### Deadzones

Raise a deadzone if a stick drifts while centered. Lower it if movement or camera control feels unresponsive near the center. Change values in small steps, save, then test in game.

### Buttons

Button mappings are shown from the game's point of view. Each row is an action the original game understands, such as a direction, confirm, back, or start/pause. The source shown next to it is the physical controller button that performs that action.

To change a mapping, choose **Bind** on the action you want to change, then press the controller button or trigger you want to use. If that button is already assigned elsewhere, the configuration tool swaps the old source into the other row so the same physical button is not shown twice.

Change one binding at a time, then test in game. If a button does the wrong thing, use **Reset** to restore the default input for that action, or **Clear** to leave it unbound.

## Advanced editing

You can edit `dttr.json` directly or start DttR with another config file:

```sh
dttr.exe path/to/my_config.json
```

See [Configuration (Technical)](technical/configuration.md) for the full JSON key reference.
