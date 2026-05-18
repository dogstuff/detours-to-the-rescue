# Building

Use the [Nix](https://github.com/NixOS/nix) shell for local Windows builds. It provides MinGW, the project dependencies, and the build tools this project expects.

!!! note

    On Windows, use WSL with the Nix flow below.

## Nix Flake

Enter the development shell, then build the debug distribution:

```sh
nix develop
just build
# Output: build/dist/Debug/
```

To configure the default local build, run:

```sh
just setup-build

# Equivalent CMake preset:
cmake --preset mingw32-nix
```

Both commands configure `build/` with `Ninja Multi-Config` and default to `DTTR_MODS_ENABLED=OFF`.

To build with modding enabled, set `DTTR_MODS_ENABLED=ON` and run the same `just` command:

```sh
DTTR_MODS_ENABLED=ON just build
# Output: build/dist/Debug-modding/
```

## Container Build

Use the project container when you need a clean toolchain image:

```sh
just build-container
# Output: build-container/dist/Debug/
```

Before sharing a build, run the local tests:

```sh
just test-all
```

Create archives with:

```sh
just package-debug
just package-release
```
