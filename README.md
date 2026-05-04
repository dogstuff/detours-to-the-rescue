# 102 Patches: Detours to the Rescue! (DttR)

DttR is an alternate entrypoint for **102 Dalmatians: Puppies to the Rescue**. It leaves the game logic intact and replaces the fragile original PC port glue with a modern runtime, so the Windows release runs more predictably on current machines.

Documentation: <https://dogstuff.gitlab.io/detours-to-the-rescue/>

## Highlights

- Direct3D 12, Vulkan, and OpenGL 3.3 graphics backends
- SDL3 controller support and modern input routing
- Stable windowed and fullscreen modes
- FFmpeg movie playback for MCI-era videos
- Audio, crash, compatibility, and filesystem fixes
- Optional modding API and builds

## Quick start

1. Download a release from <https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases>.
2. Extract it to a writable folder.
3. Run `dttr.exe`.
4. Point it at an installed game folder, the original CD, or an ISO.

Edit `dttr.json` if you want to change video, input, audio, or modding options by hand. To start with a different config file:

```sh
dttr.exe path/to/my_config.json
```

## Building

DttR uses Nix to provide the cross-compiler and build tools.

```sh
nix develop
task build
```

Container build:

```sh
task container-build
```

## License

See [LICENSE](LICENSE) for details.
