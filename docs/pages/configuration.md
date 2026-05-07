# Configuration

Run `dttr_config.exe` next to `dttr.exe` to change settings without manually editing `dttr.json`.

![DttR configuration window](assets/config-gui.png)

Save before closing the configuration tool.

## General

### Game Directory or ISO

Use **Game directory or ISO** to switch discs, choose another installed copy, or fix a moved path.

### Save Folder

DttR writes saves to `saves` next to `dttr.exe` by default.

Change it when:

- Windows cannot write to the DttR folder
- you want saves in a backed-up location
- you want separate save folders for testing or speedrunning

### Intro Movies

Enable **Skip intro movies** to skip the opening videos.

## Graphics

### Scaling Method

Use `logical` unless you are comparing renderers or debugging scaling.

- `logical` is the normal choice.
- `present` scales the final image instead.

### Scaling Fit

- `letterbox` keeps the correct aspect ratio and adds borders if needed.
- `stretch` fills the whole window, even if that distorts the image.
- `integer` scales in whole-number steps for sharper pixels.

Start with `letterbox` unless you want another look.

### Vertex Precision

- `native` keeps the original-style vertex positioning.
- `subpixel` allows smoother polygon movement, but it currently reveals seams in some models.

### Graphics API

Keep this on `auto` unless startup or rendering problems require a backend.

- `auto` lets DttR choose the best graphics API for the machine.
- `vulkan`, `direct3d12`, and `opengl` force a backend.

### Other Video Options

- `window_width` and `window_height` set the startup window size.
- `fullscreen` starts DttR fullscreen. You can also toggle fullscreen in game with ++f11++.
- `sprite_smooth` smooths scaled sprites. Turn it off for sharper pixels.
- `msaa_samples` smooths 3D edges at some performance cost.
- `generate_texture_mipmaps` can make scaled textures look smoother.

Leave texture upload synchronization at its default unless you are debugging a renderer issue.

## Audio

### Audio Output

Audio routes the game's old Miles Sound System calls through SDL. This is the tested path on modern Windows.

### Volume and Sample Tuning

- `mss_sample_gain` makes game audio louder or quieter. Use small changes; large values can clip.
- `mss_sample_preemphasis` changes how samples are filtered before playback. Leave it at the default unless you are deliberately tuning or comparing audio output.

## Gamepad

### Enable and Pick a Controller

Enable gamepad support, save, then start the game with the controller connected.

The controller index is SDL's gamepad number. Keep `0` for one controller. If DttR listens to the wrong controller, change the index, save, and restart.

### Sticks and Axes

The default layout uses the left stick for movement and the right stick for the camera. Set an axis to `none` to disable it.

Change axis bindings when:

- movement is on the wrong stick
- the camera moves on the wrong axis
- a trigger or unused stick should do nothing

### Deadzones

Raise a deadzone if a centered stick drifts. Lower it if movement or camera control feels unresponsive near the center. Change values in small steps, save, then test in game.

### Buttons

Button mappings use the game's actions: directions, confirm, back, and start/pause. The source is the physical controller button for that action.

To change a mapping, choose **Bind**, then press the controller button or trigger. If that button is already assigned elsewhere, the configuration tool swaps the old source into the other row.

Change one binding at a time, then test in game. Use **Reset** to restore the default input, or **Clear** to leave it unbound.

## Advanced Editing

You can edit `dttr.json` directly or start DttR with another config file:

```sh
dttr.exe path/to/my_config.json
```

See [Configuration (Technical)](technical/configuration.md) for the full JSON key reference.
