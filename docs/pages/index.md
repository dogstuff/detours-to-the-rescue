# 102 Patches: Detours to the Rescue! (DttR)

DttR is a launcher and runtime for 102 Dalmatians: Puppies to the Rescue that allows the PC version of the game to run well on modern systems. This includes a modding SDK that significantly eases the process of both creating and installing game modifications.

![Preview of DttR in gameplay](assets/preview.png){ width="600" }

<!-- docs-download:start -->
<!-- docs-download:end -->

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

1. On release docs, use the Vanilla or Modding Enabled download buttons above. On branch docs, pick a tagged release from [GitLab Releases](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases).
2. Extract the archive to a writable directory.
3. Run `dttr.exe`.
4. When DttR asks for the original game files, pick one source:

![DttR prompt asking where to load game files from](assets/load-prompt.png)

### Original CD

Insert the original 102 Dalmatians: Puppies to the Rescue CD, then select the disc.

![DttR loader showing an inserted game disc](assets/load-disc.png)

### Installed Copy

Choose Open Directory, then select the installed game directory with `pcdogs.exe`.

![Windows directory picker selecting an installed 102 Dalmatians directory](assets/load-install.png)

### ISO

Choose Open ISO, select the original disc image, then choose Open.

![Windows file picker selecting a 102 Puppies ISO image](assets/load-iso.png)

Once DttR finds the game files, the game starts normally.

![102 Dalmatians title screen running through DttR](assets/done.png)

Need renderer, window, audio, or controller settings? See [Configuration](configuration.md).
