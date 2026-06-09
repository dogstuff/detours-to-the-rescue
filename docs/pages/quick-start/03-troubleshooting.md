# Troubleshooting

Try these when DttR starts but crashes, renders incorrectly, or behaves differently than expected.

## Startup Crashes

First, try a different `Graphics API`. Open `dttr-config.exe`, go to **Graphics**, and change `Graphics API` from `auto` to one of these:

- `vulkan`
- `direct3d12`
- `opengl`

Save, close the config tool, then start DttR again. If one backend crashes or shows a blank window, another one may still work on the same machine.

If DttR still crashes:

- Turn off fullscreen and start in a window.
- Set `MSAA Samples` to `1`.
- If you are using the Modding build, temporarily disable third-party mods or remove them from the `mods` directory.

## Intro Movie Crashes

If DttR launches but crashes during the opening videos or before the main menu, try skipping the intro movies.

Open `dttr-config.exe`, enable `Skip Intro Movies`, save, then restart DttR.

If this fixes the crash, mention that in any bug report.

## Graphics Issues

Try switching between `Graphics API` backends first.
If the problem changes between backends, mention that in the bug report; renderer bugs are often backend-specific.

If only polygon movement or seams look wrong, try switching `Vertex Precision` between `native` and `subpixel`.

If that does not work, try these next:

- Setting `Scaling Method` back to `logical`.
- Setting `Scaling Fit` to `letterbox`.
- Turning `Sprite Smooth` off and/or setting `MSAA Samples` to `1`.

After each config change, save and restart the game.

## Controller Input Issues

Open `dttr-config.exe`, turn on `Enable Gamepad`, save, then restart DttR with the controller already connected.

If DttR listens to the wrong controller, try changing `Gamepad Index`. If a stick drifts or feels unresponsive, adjust the matching deadzone field in small steps.

## Audio Issues

Open `dttr-config.exe` and lower `MSS Sample Gain`. Very high gain values can clip and sound distorted.

## More Help

If you are still having problems, report the issue through [DttR's GitLab repository](https://gitlab.com/dogstuff/detours-to-the-rescue).

Include:

- The DttR version you are using.
- Whether you downloaded Vanilla or Modding.
- Your selected `Graphics API`.
- Your game source: CD, installed copy, or ISO.
- `dttr.log`, with `Log Level` set to `debug` or `trace`.
- If requested, the relevant crash `.dmp` files created by DttR. Crash dumps may contain sensitive information, so do not post them publicly unless asked.
