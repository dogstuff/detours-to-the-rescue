# 102 Patches: Detours to the Rescue

In this game, you play as detours and rescue 102 Dalmatians from outdated dependencies and incompatibilities created by the evil Microsoft de RectX7 (not joking).

![DttR Preview](static/preview.png){width=50% height=100px}

---

## Overview

DttR is an all-in-one alternate entrypoint that aims to make 102 Dalmatians: Puppies to the Rescue as portable and consistent as possible. It includes:

- Support for modern graphics APIs (Direct3D 12, Vulkan, OpenGL)
- Native controller support
- Stable windowed and fullscreen modes
- Various other fixes for crashes and undesired behavior

DttR implements purpose-built translators for these older APIs:

- DirectDraw7 and Direct3D7 -> Direct3D 12, Vulkan (SDL3 GPU), OpenGL (OpenGL 3.3)
  - Optionally includes a range of graphics-related optimizations and improvements
- DirectInput and WinAPI input -> SDL3
- Media Control Interface (MCI) -> libmpv2
- More to come, probably

This project was created with the [102 Dalmatians Speedrunning Community](https://www.102.dog/) in mind, so it aims to modify as little of the original game logic as possible.

Internal documentation is available [here](https://dogstuff.gitlab.io/detours-to-the-rescue/).

---

## Tested Game Versions

All known retail PC releases of 102 Dalmatians: Puppies to the Rescue are supported:

- English
- French, German, Italian, Spanish, Dutch (European)
- Norwegian, Danish, Swedish (Scandinavian)

---

## Getting started

1. Download the latest DttR build from the [release page](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases).
2. Extract the archive to a folder of your choice.
3. Run `dttr.exe`.
4. Select the game installation directory that contains `pcdogs.exe`.
5. Profit.

You can edit the included `dttr.jsonc` configuration file to change video settings, gamepad input, and other options.

To use a specific configuration file, pass it as an argument:

```sh
dttr.exe path/to/my_config.jsonc
```

---

## Components

Components are optional, dynamically loaded plugins that extend DttR without modifying core code.
They live in `{my_dttr_dir}/components/` and load automatically at runtime.

Components have access to the same hooking, patching, and scanning APIs as the core sidecar code.

### Components Builds

The **normal build** (`dttr-vX.X.X-release.zip`) does not include the component loading system. This version is intended to run the visual game with as few modifications as possible.

The **components build** (`dttr-components-vX.X.X-release.zip`) includes the component loading system and all bundled components.
This version is **not** allowed on the speedrun.com leaderboards and currently overlays a watermark.

Each release provides both build variants as separate downloads.

---

## Compilation

This project's build system relies on [Nix](https://github.com/NixOS/nix) for toolchain and dependency management.

Sorry, Windows users; you can figure this out on your own.

### Nix flake

```sh
nix develop
task build
# Output: build/dist/
```

This pulls in the cross-compiler, SDL3, VLC, NASM, and the remaining build dependencies automatically.

### Container build

Build the project inside a container:

```sh
task container-build
# Output: build-container/dist/
```

---

## License

See [LICENSE](LICENSE) for details.
