# 102 Patches: Detours to the Rescue! (DttR)

DttR is an alternate entrypoint for **102 Dalmatians: Puppies to the Rescue**. It leaves the game logic intact and replaces the fragile original PC port glue with a modern runtime, so the Windows release runs more predictably on current machines.

![Preview of DttR in gameplay](assets/preview.png){ width="600" }

## Highlights

- Direct3D 12, Vulkan, and OpenGL 3.3 graphics backends
- Stable windowed and fullscreen modes
- Native controller support
- Modern keyboard input routing
- FFmpeg movie playback for MCI-era videos
- Audio, crash, compatibility, and filesystem fixes
- Optional modding API and builds

DttR is built with the [102 Dalmatians Speedrunning Community](https://www.102.dog/) in mind. The goal is portability and stability, not changing how the game plays.

## Supported releases

The following PC releases of **102 Dalmatians: Puppies to the Rescue** are supported:

- English
- French, German, Italian, Spanish, Dutch (European)
- Norwegian, Danish, Swedish (Scandinavian)

## Quick start

1. Download the [latest normal release build](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-v1.7.1-release.zip).
2. Extract the archive to a writable folder.
3. Run `dttr.exe`.
4. Choose where DttR should load the original game files from:

![DttR prompt asking where to load game files from](assets/load-prompt.png)

### Original CD

Insert the **102 Dalmatians: Puppies to the Rescue** CD, then choose the detected disc button.

![DttR loader showing an inserted game disc](assets/load-disc.png)

### Installed copy

Choose **Open Directory**, then pick the folder that contains the game's `data` directory.

![Windows folder picker selecting an installed 102 Dalmatians folder](assets/load-install.png)

### ISO

Choose **Open ISO**, select the original disc image, then choose **Open**.

![Windows file picker selecting a 102 Puppies ISO image](assets/load-iso.png)

If all is well, you're done!

![PttR title screen running through DttR](assets/done.png)

Issues setting up or need to adjust the renderer, window, audio, or controller setup? See [Configuration](configuration.md).
