# 102 Patches: Detours to the Rescue


In this game you play as detours and you have to rescue 102 Dalmatians from the outdated dependencies and
incompatibilities made by the evil Microsoft de RectX7 (not joking)

![DttR Preview](static/preview.png){width=50% height=100px}

---

## Overview

An all-in-one alternate entrypoint that seeks to make 102 Dalmatians: Puppies to the Rescue as portable and consistent as possible, including:

* Support for modern graphics API (Direct3D12, Vulkan, OpenGL)
* Native controller support
* Stable support for windowed and fullscreen modes
* Various other fixes for crashes and undesired behavior

DttR implements purpose-built translators for the following outdated APIs:

* DirectDraw7 & Direct3D7 -> Direct3D12, Vulkan (SDL3 GPU), OpenGL (OpenGL 3.3)
    * Optionally uses a bunch of graphics-related optimizations and improvements
* DirectInput & WinAPI Inputs -> SDL3
* Media Control Interface (MCI) -> libmpv2
* More to come probably

Because this project was created with the [102 Dalmatians Speedrunning Community](https://www.102.dog/) in mind,
it aims to modify as little of the original game logic as possible.

The generated internal documentation for this project can be found [here](https://dogstuff.gitlab.io/detours-to-the-rescue/).

---

## Tested Game Versions

All known retail PC releases of 102 Dalmatians: Puppies to the Rescue are supported:

* English
* French, German, Italian, Spanish, Dutch (European)
* Norwegian, Danish, Swedish (Scandinavian)

---

## Getting started

1. Download the latest DttR build from the [release page](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases).
2. Extract the archive to a folder of your choice.
3. Run `dttr.exe`.
4. Select the game installation directory containing `pcdogs.exe`.
5. Profit.

The included `dttr.jsonc` configuration file can be edited to modify video settings, gamepad input, etc.

A specific configuration file can be used by passing it as an argument:

```sh
dttr.exe path/to/my_config.jsonc
```

---

## Components

Components are optional, dynamically-loaded plugins that extend DttR without modifying core code. 
They live in the  `{my_dttr_dir}/components/` and are auto-loaded at runtime.

Components have access to the same hooking, patching, and scanning APIs as the core sidecar code.

### "Components" Builds

The **normal build** (`dttr-vX.X.X-release.zip`) does not include the component loading system. This version is intended to run the visual game with as few modifications as possible.

The **components build** (`dttr-components-vX.X.X-release.zip`) includes the component loading system and all components bundled with the game.
This version is NOT allowed on the speedrun.com leaderboards and as such currently overlays a watermark.

Components builds are provided as separate downloads for each release.

---

## Compilation

This project's build system relies on [Nix](https://github.com/NixOS/nix) for toolchain and dependency management.

(sorry windows users you can figure this out on your own)

### Nix Flake

```sh
nix develop
task build
# Output: build/dist/
```

This pulls in the cross-compiler, SDL3, VLC, NASM, and everything else automatically.

### Container Build

Build the project inside a container:

```sh
task container-build
# Output: build-container/dist/
```

---

## License

See [LICENSE](LICENSE) for details.
