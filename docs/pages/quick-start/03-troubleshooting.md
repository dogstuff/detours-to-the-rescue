# Troubleshooting

Try these when DttR starts but crashes, renders incorrectly, or behaves differently than expected.

## Game crashes on startup

First, try a different graphics API. Open `dttr-config.exe`, go to **Graphics**, and change **Graphics API** from `auto` to one of these:

- `vulkan`
- `direct3d12`
- `opengl`

Save, close the config tool, then start DttR again. If one backend crashes or shows a blank window, another one may still work on the same machine.

If DttR still crashes:

- Turn off fullscreen and start in a window.
- Set MSAA samples to `1`.
- If you are using the Modding build, temporarily disable third-party mods or remove them from the `mods` directory.

## Game crashes during intro movies

If DttR launches but crashes during the opening videos or before the main menu, try skipping the intro movies.

Open `dttr-config.exe`, enable **Skip intro movies**, save, then restart DttR.

If this fixes the crash, mention that in any bug report.

## Graphics are missing, flickering, or otherwise broken

To start, tery switching between Graphics APIs and check if the bug is resolved.
If not, does the glitched behavior vary between APIs? 
Renderer bugs can often be specific to certain Graphics API backend implementations.

If only polygon movement or seams look wrong, try switching **Vertex Precision** between `native` and `subpixel`.

If that doesn't work, some other diagnostic steps include:

- Setting `Scaling Mode` back to `logical`.
- Setting `Scaling Fit` to `letterbox`.
- Disabling sprite smoothing and/or MSAA

For each change to the config, ensure your config changes are saved and the game has been resterted.

## Controller input is wrong or missing

Open `dttr-config.exe`, enable gamepad support, save, then restart DttR with the controller already connected.

If DttR listens to the wrong controller, try changing the controller index. If a stick drifts or feels unresponsive, adjust the deadzone in small steps.

## Audio is distorted or too loud

Open `dttr-config.exe` and lower the audio gain. Very high gain values can clip and sound distorted.

## Still having issues?

If you're still having problems, feel free to report the issue through [DttR's GitLab repository](https://gitlab.com/dogstuff/detours-to-the-rescue).

Be sure to include:

- The DttR version you are using.
- Whether you downloaded Vanilla or Modding.
- Your selected graphics API.
- Your game source: CD, installed copy, or ISO.
- `dttr.log`; configured `Log level` should be `debug` or `trace`.
- If requested, please also provide the relevant crash `.dmp` files created by DttR. **Note: Crash dumps may contain sensitive information and should generally not be posted publicly.**
