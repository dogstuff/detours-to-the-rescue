# 102 Patches: Detours to the Rescue


In this game you play as detours and you have to rescue 102 Dalmatians from the outdated dependencies and
incompatibilities made by the evil Microsoft de RectX7 (not joking)

![DttR Preview](static/preview.png){width=50% height=100px}

---

## Overview

An all-in-one alternate entrypoint that seeks to make 102 Dalmatians: Puppies to the Rescue as portable and consistent as possible, including:

* Support for modern graphics API (Direct3D12, Vulkan, Metal)
* Native controller support
* Stable support for windowed and fullscreen modes
* Various other fixes for crashes and undesired behavior

DttR implements purpose-built translators for the following outdated APIs:

* DirectDraw7 & Direct3D7 -> Direct3D12, Vulkan, Metal (backed by SDL3GPU)
    * Optionally uses a bunch of graphics-related optimizations and improvements
* DirectInput & WinAPI Inputs -> SDL3
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
