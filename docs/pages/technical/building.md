# Building

DttR uses [Nix](https://github.com/NixOS/nix) for the cross-compiler, tools, and libraries used by the documented builds.

!!! note

    Native Windows build steps are not documented yet. Windows users should build in WSL for now.

## Nix Flake

Build from the development shell:

```sh
nix develop
task build
# Output: build/dist/
```

The shell brings in the cross-compiler, SDL3, FFmpeg, NASM, and the rest of the build dependencies.

## Container Build

Build inside a container:

```sh
task container-build
# Output: build-container/dist/
```
