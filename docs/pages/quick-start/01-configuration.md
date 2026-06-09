# Configuration

Run `dttr-config.exe` next to `dttr.exe` to change settings without hand-editing `dttr.json`.

![DttR configuration window](../assets/config-gui.png)

Choose **Save** before closing the tool. DttR reads the saved `dttr.json` the next time it starts, so restart the game after changing settings.

## Switch Game Files

Use `Game Directory or ISO` to switch discs, choose another installed copy, fix a path after moving files, or open the first-time setup prompt again.

- For an installed copy, choose the folder that contains `pcdogs.exe`.
- For an ISO, choose the original disc image.
- For a disc, insert or mount it before opening DttR.

## Move Save Files

By default, DttR reads and writes saves from a DttR-owned directory (`saves` next to `dttr.exe`). Each game executable variant gets its own subdirectory.

If you need the game to use its original save paths, set `saves_path` to an empty string in `dttr.json`.

## Skip Intro Movies

Turn on `Skip Intro Movies` to skip the opening videos. This is also worth trying if the game crashes before reaching the main menu.

## Graphics

Start with the defaults unless you are fixing a specific problem or want a different look.

If the game has startup or rendering problems, try changing `Graphics API`:

- `auto` lets DttR choose.
- `vulkan`, `direct3d12`, or `opengl` force one backend.

Use `Scaling Fit` to control the window shape:

- `letterbox` keeps the correct aspect ratio.
- `stretch` fills the window but may distort the image.
- `integer` uses whole-number scaling for sharper, more consistent pixels.

Use `Scaling Method` only when you need to tune where scaling happens:

- `logical` scales the game's internal image.
- `present` scales the final image shown on your display.

If polygons shimmer or seams look wrong with `logical` scaling, try changing `Vertex Precision`. Use `native` for the original positioning style, or `subpixel` for smoother movement with possible minor artifacts.

Other useful graphics settings:

- Turn `Fullscreen` on to start fullscreen. You can also toggle fullscreen in game with ++f11++.
- Turn `Sprite Smooth` off for sharper sprite pixels.
- Set `MSAA Samples` to `1` to disable MSAA.

## Controller

Turn on `Enable Gamepad`, save your changes, then start DttR with the controller connected.

If DttR uses the wrong controller, change `Gamepad Index`; keep it at `0` when only one controller is connected.

If movement or camera control feels off, adjust the axis bindings or the matching deadzone fields: raise the deadzone if a centered stick drifts, and lower it if movement feels unresponsive near the center.

To change a button, click its `Bind` button, then press the controller button or trigger you want to use.

Use `Reset` to restore the default mapping, or `Clear` to leave an action unbound.

## Audio

Use `MSS Sample Gain` to make game audio louder or quieter.

Leave `MSS Sample Preemphasis` at the default unless you have a specific need to adjust it.

## Logs and Crash Reports

Use `Log File Path` to configure where DttR should write `dttr.log`. Relative paths are based on the DttR directory.

If you need more detail for troubleshooting, set `Log Level` to `debug` or `trace`.

Use `Minidump Type` to control how much information crash dumps include. Posting crash dumps publicly can expose sensitive information; share them only when requested.
## Modding Options

The Modding tab is experimental and only exists in the Modding build.

- Use `Hot Reload` to enable reloading updated mod DLLs automatically without needing to restart the game.
- To disable specific installed mods, uncheck their DLLs in the mod list.

## Edit the Config File

For direct JSON editing, alternate config files, and the full key reference, see [Configuration (Technical)](../technical-reference/00-configuration.md).
