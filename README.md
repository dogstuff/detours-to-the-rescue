# 102 Patches: Detours to the Rescue! (DttR)

DttR is an alternate entrypoint for **102 Dalmatians: Puppies to the Rescue**. It leaves the game logic intact and replaces the fragile original PC port glue with a modern runtime, so the Windows release runs more predictably on current machines.

Documentation: <https://dogstuff.gitlab.io/detours-to-the-rescue/>

## Features

- Direct3D 12, Vulkan, and OpenGL 3.3 graphics backends
- SDL3 controller support and modern input routing
- Stable windowed and fullscreen modes
- FFmpeg movie playback for MCI-era videos
- Audio, crash, compatibility, and filesystem fixes
- A flexible API to facilitate game modifications

## License

See [LICENSE](LICENSE) for details.
