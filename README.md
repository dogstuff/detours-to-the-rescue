# 102 Patches: Detours to the Rescue

DttR is an alternate entrypoint for **102 Dalmatians: Puppies to the Rescue** that makes PC release more portable, stable, and consistent on modern systems.

![DttR preview](static/preview.png)

---

## Features

- Modern graphics backends:
  - Direct3D 12
  - Vulkan
  - OpenGL 3.3
- Optional graphics optimizations and visual improvements
- Native controller support through SDL3
- Modernized input handling
- Stable windowed and fullscreen modes
- MCI movie playback through FFmpeg
- Crash fixes and compatibility patches

This project was created with the [102 Dalmatians Speedrunning Community](https://www.102.dog/) in mind, so it aims to modify as little of the original game logic as possible.

---

## Supported Versions

The following PC releases of **102 Dalmatians: Puppies to the Rescue** are supported:

- English
- French, German, Italian, Spanish, Dutch (European)
- Norwegian, Danish, Swedish (Scandinavian)

---

## Installation

1. Download the latest DttR build from the [release page](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases).
2. Extract the archive to a folder of your choice.
3. Run `dttr.exe`.
4. Select the game installation directory that contains `pcdogs.exe`.
5. Profit.

---

## Configuration

Edit the included `dttr.jsonc` file to change video settings, gamepad input bindings, and other options.

To use a specific configuration file, pass it as an argument:

```sh
dttr.exe path/to/my_config.jsonc
```

---

## Components

> [!WARNING]
> The components API is experimental. It is incomplete and breaking changes may be made without warning.

Components are optional, dynamically loaded plugins that extend DttR without modifying core code.
They live in `{my_dttr_dir}/components/` and load automatically at runtime.

Components have access to the same hooking, patching, and scanning APIs as the core sidecar code.

### Build Variants

- **Normal build** (`dttr-vX.X.X-release.zip`)
  - Does not include the component loading system.
  - Intended to run the vanilla game with as few modifications as possible.
- **Modding build** (`dttr-modding-vX.X.X-release.zip`)
  - Includes the component loading system.
  - Not allowed on the speedrun.com leaderboards.

Each release provides both build variants as separate downloads.

---

## Building

This project's build system relies on [Nix](https://github.com/NixOS/nix) for toolchain and dependency management.

Sorry, Windows users; you can figure this out on your own.

Internal documentation is available [here](https://dogstuff.gitlab.io/detours-to-the-rescue/).

### Nix Flake

```sh
nix develop
task build
# Output: build/dist/
```

This pulls in the cross-compiler, SDL3, FFmpeg, NASM, and the remaining build dependencies automatically.

### Container Build

Build the project inside a container:

```sh
task container-build
# Output: build-container/dist/
```

---

## License

See [LICENSE](LICENSE) for details.
