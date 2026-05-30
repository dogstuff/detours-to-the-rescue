# Creating a Mod

A DttR mod is a 32-bit Windows DLL that includes `dttr_sdk.h`, exports mod lifecycle symbols, and links against a build of the DttR SDK.

The easiest starting point is the C template from the latest release:

[`dttr-mod-template-c.tar.gz`](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-mod-template-c.tar.gz)

## Starting from the project template

Download and extract the template. It includes the CMake project, MinGW toolchain file, SDK linking scripts, container build, and a minimal mod source file.

The starter mod is intentionally small:

```c
#include <dttr_sdk.h>

DTTR_MODS_INFO("minimal", "0.1.0", "DttR")

static const DTTR_Mods_Context *mod_ctx;

DTTR_MODS_INIT {
    mod_ctx = ctx;
    DTTR_MODS_LOG_INFO(ctx, "Hello world!");
    return true;
}

DTTR_MODS_CLEANUP {
    DTTR_MODS_LOG_INFO(mod_ctx, "Goodbye world o/");
}
```

## Setting up the build environment

Install the build tools for your platform.

If you use Nix on Linux, macOS, or WSL, enter the template development shell:

```sh
nix develop
```

On APT-based Linux distributions:

```sh
sudo apt update
sudo apt install cmake ninja-build curl unzip gcc-mingw-w64-i686
```

On DNF-based distributions:

```sh
sudo dnf install cmake ninja-build curl unzip mingw32-gcc
```

On macOS with Homebrew:

```sh
brew install cmake ninja curl unzip mingw-w64
```

On Windows with `winget`:

Install MSYS2 if needed:

```powershell
winget install --exact --id MSYS2.MSYS2
```

Then open an MSYS2 MINGW shell and install the build tools:

```sh
pacman -S --needed mingw-w64-i686-cmake mingw-w64-i686-ninja mingw-w64-i686-gcc curl unzip git
```

If you do not want a local MinGW install, use the template's Podman container build instead.
The template README has the container build details.

## Fetching the DttR SDK

!!! note

    You can skip this step when using the default container build because it fetches the SDK for you.

The template downloads a build of the release SDK that matches `dttr-version.txt`.

On Linux, macOS, or inside the Nix shell:

```sh
./scripts/fetch-dttr.sh
```

On Windows with PowerShell:

```powershell
./scripts/fetch-dttr.ps1
```

## Compiling the mod DLL

On Linux, macOS, or inside the Nix shell:

```sh
cmake -S . -B build -G "Ninja Multi-Config" \
  -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
  -DDTTRSDK_DIR=.dttr/sdk

cmake --build build --config debug
```

On Windows using the MSYS2 MINGW shell:

```sh
cmake -S . -B build -G "Ninja Multi-Config" -DDTTRSDK_DIR=.dttr/sdk

cmake --build build --config debug
```

The built mod should be here:

```text
build/debug/minimal-mod.dll
```
