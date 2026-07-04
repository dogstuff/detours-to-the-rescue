---
title: "Project Setup (Creating a Mod)"
description: "Create a C mod project with the template, SDK headers, CMake build, and 32-bit Windows DLL output."
seo_type: "article"
---

# Project Setup

A DttR mod is a 32-bit Windows DLL that includes `dttr_sdk.h`, exports mod lifecycle symbols, and links against a build of the DttR SDK.

The easiest starting point is the C template from the stable release:

[`dttr-mod-template-c.zip` (__DTTR_DOCS_STABLE_VERSION__)](__DTTR_DOCS_MOD_TEMPLATE_STABLE_DOWNLOAD_URL__)

## Project Template

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

## Build the Mod DLL

Choose the tab for the system you are building on. Each tab includes the full setup, SDK fetch, and build flow for that system.

=== "Nix"

    **Set up the build environment**

    On Linux, macOS, or WSL with Nix, enter the template development shell:

    ```sh
    nix develop
    ```

    **Fetch the DttR SDK**

    The template downloads a build of the release SDK that matches `dttr-version.txt`:

    ```sh
    ./scripts/fetch-dttr.sh
    ```

    **Compile the mod DLL**

    ```sh
    cmake -S . -B build -G "Ninja Multi-Config" \
      -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
      -DDTTRSDK_DIR=.dttr/sdk

    cmake --build build --config debug
    ```

=== "APT"

    **Set up the build environment**

    On APT-based Linux distributions:

    ```sh
    sudo apt update
    sudo apt install cmake ninja-build curl unzip gcc-mingw-w64-i686
    ```

    **Fetch the DttR SDK**

    The template downloads a build of the release SDK that matches `dttr-version.txt`:

    ```sh
    ./scripts/fetch-dttr.sh
    ```

    **Compile the mod DLL**

    ```sh
    cmake -S . -B build -G "Ninja Multi-Config" \
      -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
      -DDTTRSDK_DIR=.dttr/sdk

    cmake --build build --config debug
    ```

=== "DNF"

    **Set up the build environment**

    On DNF-based Linux distributions:

    ```sh
    sudo dnf install cmake ninja-build curl unzip mingw32-gcc
    ```

    **Fetch the DttR SDK**

    The template downloads a build of the release SDK that matches `dttr-version.txt`:

    ```sh
    ./scripts/fetch-dttr.sh
    ```

    **Compile the mod DLL**

    ```sh
    cmake -S . -B build -G "Ninja Multi-Config" \
      -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
      -DDTTRSDK_DIR=.dttr/sdk

    cmake --build build --config debug
    ```

=== "Homebrew (macOS)"

    **Set up the build environment**

    On macOS with Homebrew:

    ```sh
    brew install cmake ninja curl unzip mingw-w64
    ```

    **Fetch the DttR SDK**

    The template downloads a build of the release SDK that matches `dttr-version.txt`:

    ```sh
    ./scripts/fetch-dttr.sh
    ```

    **Compile the mod DLL**

    ```sh
    cmake -S . -B build -G "Ninja Multi-Config" \
      -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
      -DDTTRSDK_DIR=.dttr/sdk

    cmake --build build --config debug
    ```

=== "winget (Windows)"



    !!! info ""

        Open an MSYS2 MINGW32 shell before installing or building. If you only see a MINGW64 shortcut, or no MINGW32 shortcut at all, open `C:\msys64\mingw32.exe` directly.

        If you want other shells and applications to find the installed MINGW32 tools, add these entries to your Windows `Path` user variable:

        - `C:\msys64\mingw32\bin`
        - `C:\msys64\usr\bin`

    **Set up the build environment**

    Install MSYS2 if needed:

    ```powershell
    winget install --exact --id MSYS2.MSYS2
    ```

    Install the build tools:

    ```sh
    pacman -S --needed mingw-w64-i686-cmake mingw-w64-i686-ninja mingw-w64-i686-gcc curl unzip git
    ```

    **Fetch the DttR SDK**

    In MSYS2:

    ```shell
    ./scripts/fetch-dttr.sh
    ```


    *or* in PowerShell:

    ```powershell
    ./scripts/fetch-dttr.ps1
    ```

    **Compile the mod DLL**

    In the MSYS2 MINGW32 shell, or a new Windows terminal after configuring `Path` for MINGW32:

    ```sh
    cmake -S . -B build -G "Ninja Multi-Config" -DDTTRSDK_DIR=.dttr/sdk

    cmake --build build --config debug
    ```

=== "Container"

    **Set up the build environment**

    Install your containerization software of choice.

    **Fetch the DttR SDK**

    !!! info ""

        You can skip this step when using the default container build because it fetches the SDK for you.

    If you pass `--build-arg DTTR_FETCH_SDK=0`, fetch the SDK first so the local SDK files exist in the build context:

    ```sh
    ./scripts/fetch-dttr.sh
    ```

    **Compile the mod DLL**

    Build the template into a local container image, then copy the compiled DLL out of the artifact image:

    ```sh
    # Build the template into a local container image.
    podman build -t dttr-minimal-mod -f Containerfile .

    # Create a container from that image so we can copy files out of it.
    container_id=$(podman create dttr-minimal-mod)

    # Copy the built DLL from the container into the same output path as local builds.
    mkdir -p build/debug
    podman cp "$container_id:/build/debug/minimal-mod.dll" build/debug/minimal-mod.dll

    # Remove the temporary container.
    podman rm "$container_id"
    ```

The built mod should be here:

```text
build/debug/minimal-mod.dll
```
