# 102 Patches: Detours to the Rescue! (DttR)

DttR is an alternate entrypoint for **102 Dalmatians: Puppies to the Rescue** that replaces the fragile original PC port glue with a complete modern runtime, allowing the game to run seamlessly on modern machines.

![Preview of DttR in gameplay](assets/preview.png){ width="600" }

## Highlights

- Direct3D 12, Vulkan, and OpenGL 3.3 graphics backends
- Stable windowed and fullscreen modes
- Native controller support
- Modern keyboard input routing
- FFmpeg movie playback for MCI-era videos
- Audio, crash, compatibility, and filesystem fixes
- Optional modding support

DttR is built for the [102 Dalmatians Speedrunning Community](https://www.102.dog/). It aims for portability and stability, not gameplay changes.

## Supported PC releases

- English
- French, German, Italian, Spanish, Dutch (European)
- Norwegian, Danish, Swedish (Scandinavian)

## Quick start

1. Download the [latest release build](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-release.zip).
2. Extract the archive to a writable folder.
3. Run `dttr.exe`.
4. When DttR asks where to load the original game files from, choose one of these:

![DttR prompt asking where to load game files from](assets/load-prompt.png)

### Original CD

Insert the original **102 Dalmatians: Puppies to the Rescue** CD, then choose the detected disc.

![DttR loader showing an inserted game disc](assets/load-disc.png)

### Installed copy

Choose **Open Directory**, then pick the folder that contains the game's `data` directory.

![Windows folder picker selecting an installed 102 Dalmatians folder](assets/load-install.png)

### ISO

Choose **Open ISO**, select the original disc image, then choose **Open**.

![Windows file picker selecting a 102 Puppies ISO image](assets/load-iso.png)

Once DttR finds the game files, it starts normally.

![PttR title screen running through DttR](assets/done.png)

For renderer, window, audio, or controller settings, see [Configuration](configuration.md).
