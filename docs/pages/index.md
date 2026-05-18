# 102 Patches: Detours to the Rescue! (DttR)

DttR is a modern launcher for **102 Dalmatians: Puppies to the Rescue** on PC. It leaves the game data intact and replaces the old Windows support layer with a runtime that works better on current systems.

![Preview of DttR in gameplay](assets/preview.png){ width="600" }

## Highlights

- Direct3D 12, Vulkan, and OpenGL 3.3 renderers
- Windowed and fullscreen play
- Native controller support
- Keyboard input tied to the DttR window
- FFmpeg playback for the game's MCI-era videos
- Fixes for audio, crashes, compatibility, and filesystem behavior
- Optional modding support

DttR is built for the [102 Dalmatians Speedrunning Community](https://www.102.dog/). It prioritizes portability and stability. By default, gameplay stays unchanged.

## Supported PC Releases

- English
- French, German, Italian, Spanish, Dutch (European)
- Norwegian, Danish, Swedish (Scandinavian)

## Quick Start

1. Download the latest release build:
   <https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-release.zip>
2. Extract the archive to a writable directory.
3. Run `dttr.exe`.
4. When DttR asks for the original game files, pick one source:

![DttR prompt asking where to load game files from](assets/load-prompt.png)

### Original CD

Insert the original **102 Dalmatians: Puppies to the Rescue** CD, then select the disc.

![DttR loader showing an inserted game disc](assets/load-disc.png)

### Installed Copy

Choose **Open Directory**, then select the installed game directory with `pcdogs.exe`.

![Windows directory picker selecting an installed 102 Dalmatians directory](assets/load-install.png)

### ISO

Choose **Open ISO**, select the original disc image, then choose **Open**.

![Windows file picker selecting a 102 Puppies ISO image](assets/load-iso.png)

Once DttR finds the game files, the game starts normally.

![102 Dalmatians title screen running through DttR](assets/done.png)

Need renderer, window, audio, or controller settings? See [Configuration](configuration.md).
