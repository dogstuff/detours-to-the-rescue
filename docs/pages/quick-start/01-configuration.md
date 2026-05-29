# Configuration

Run `dttr-config.exe` next to `dttr.exe` to change settings without hand-editing `dttr.json`.

![DttR configuration window](../assets/config.png)

Choose **Save** before closing the tool. DttR reads the saved `dttr.json` the next time it starts, so restart the game after changing settings.

## Switching game files

Use `Game directory or ISO` when you need to switch discs, choose another installed copy, fix a path after moving files, or re-access the first time setup prompt.

- For an installed copy, choose the folder that contains `pcdogs.exe`.
- For an ISO, choose the original disc image.
- For a disc, insert or mount it before opening DttR.

## Moving your saves

By default, DttR reads and writes saves from a DttR-owned directory (default `saves` next to `dttr.exe`). Each game executable variant is generated its own subdirectory within the saves directory.

If you need the game to use its original save paths, set `saves_path` to an empty string in `dttr.json`.

## Skipping intro movies

Turn on `Skip intro movies` to skip the opening videos. This is also worth trying if the game crashes before reaching the main menu.

## Adjusting graphics

Start with the defaults unless you are fixing a specific problem or prefer a different look.

Use `Graphics API` when DttR has startup or rendering problems:

- `auto` lets DttR choose the backend.
- `vulkan`, `direct3d12`, and `opengl` force a specific backend.

Use `Scaling Fit` to control the window shape:

- `letterbox` keeps the correct aspect ratio.
- `stretch` fills the whole window, even if it distorts the image.
- `integer` scales in whole-number steps for sharper and more consistent pixels.

Use `Scaling Method` to choose where DttR applies scaling:

- `logical` scales the game's internal image before presentation.
- `present` scales the final presented image. This can be useful when comparing output or tuning how scaling looks on your display.

When using `logical` scaling, the `Vertex Precision` can be changed if polygon movement or seams shimmer or otherwise look wrong . Try `native` for original positioning method or `subpixel` for smooth polygon movement with some minor visual artifacts.

Other useful graphics settings:

- Turn `Fullscreen` on to start fullscreen. You can also toggle fullscreen in game with ++f11++.
- Turn `Sprite Smooth` off for sharper pixels on sprites.
- Set `MSAA samples` to `1` to disable MSAA.

## Configuring a controller

Enable gamepad support, save, then start DttR with the controller connected.

If DttR listens to the wrong controller, change the controller index. Keep `0` when you only have one controller connected.

If movement or camera control feels wrong:

- Change stick and axis bindings when movement is on the wrong stick or camera control is on the wrong axis.
- Raise a deadzone if a centered stick drifts.
- Lower a deadzone if movement feels unresponsive near the center.

To change buttons, click the corresponding `Bind` button, then press the desired controller button or trigger.

Use `Reset` to restore the default mapping, or `Clear` to leave an action unbound.

## Adjusting audio

Use audio gain to make game audio louder or quieter.

Leave sample preemphasis at the default unless you have a specific need to adjust it.

## Collecting logs and crash reports

Use `Log file path` to move `dttr.log`. Relative paths resolve from the DttR directory.

Set `Log level` to `debug` or `trace` when you need more diagnostic detail in the generated log file.

Use `Minidump type` to choose how much detail crash dumps include. Crash dumps may contain sensitive information, so avoid posting them publicly unless requested.

## Configuring modding features

The Modding tab is experimental and only applies to the Modding build.

- Use hot reload to enable reloading updated mod DLLs automatically without needing to restart the game.
- To disable specific installed mods, the mod list to disable DLLs in the `mods` directory.

## Editing the config file directly

For direct JSON editing, alternate config files, and the full key reference, see [Configuration (Technical)](../technical-reference/00-configuration.md).
