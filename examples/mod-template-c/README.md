# DttR Minimal C Mod Template

A small cross-compilable C template project for writing [Detours to the Rescue](https://gitlab.com/dogstuff/detours-to-the-rescue) mods with MinGW.

## Quick Start

### 1. Get the template

Download and extract the template archive from the latest DttR release:

[`dttr-mod-template-c.tar.gz`](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-mod-template-c.tar.gz)

### 2. Configure a build environment

Follow the steps for the machine you are building on:

<details>
<summary><strong>Nix (Linux/macOS/WSL)</strong></summary>

With Nix, enter the template's development shell to install and configure the required toolchain:

```sh
nix develop
```

</details>

<details>
<summary><strong>APT-Based Linux Distributions (Ubuntu, Debian, etc.)</strong></summary>

On APT-based Linux distributions, run:

```sh
sudo apt update
sudo apt install cmake ninja-build curl unzip gcc-mingw-w64-i686
```

</details>

<details>
<summary><strong>RPM-Based Linux Distributions using DNF (Fedora, RHEL, etc.)</strong></summary>

On RPM-based Linux distributions with DNF, run:

```sh
sudo dnf install cmake ninja-build curl unzip mingw32-gcc
```

</details>

<details>
<summary><strong>macOS (Homebrew)</strong></summary>

On macOS with Homebrew, run:

```sh
brew install cmake ninja curl unzip mingw-w64
```

</details>

<details>
<summary><strong>Windows (MSYS2)</strong></summary>

Optional: if you do not have MSYS2, you can install it from PowerShell using winget:

```powershell
winget install --exact --id MSYS2.MSYS2
```

!!! info "Opening MINGW32"

    Open an MSYS2 MINGW32 shell before installing or building. If you only see a MINGW64 shortcut, or no MINGW32 shortcut at all, open `C:\msys64\mingw32.exe` directly.

    If you want normal Windows terminals to find the pacman-installed MINGW32 tools, add these entries to your Windows `Path` user variable:

    ```text
    C:\msys64\mingw32\bin
    C:\msys64\usr\bin
    ```

Install the build tools:

```sh
pacman -S --needed mingw-w64-i686-cmake mingw-w64-i686-ninja mingw-w64-i686-gcc curl unzip git
```

</details>

<details>
<summary><strong>Container Build</strong></summary>

If you do not want a local MinGW install, use the template's container build instead. The container image installs the compiler and build tools for you.

```sh
podman --version
```

</details>

### 3. Fetch the DttR SDK

This project includes scripts that download the DttR release SDK matching `dttr-version.txt`.

If you are using the default Podman container build in step 4, you can skip this step; the container build runs the fetch script inside the image. If you pass `--build-arg DTTR_FETCH_SDK=0`, run this step first so the local SDK files exist in the build context.

On Linux, macOS, or inside the Nix shell:

```sh
./scripts/fetch-dttr.sh
```

On Windows (PowerShell):

```powershell
./scripts/fetch-dttr.ps1
```

After a successful download, this file should exist:

```
.dttr/sdk/DTTRSDKConfig.cmake
```

Note: Some file managers hide `.dttr` unless hidden files are enabled.

### 4. Build the DLL

On Linux, macOS, or inside the Nix shell:

```sh
cmake -S . -B build -G "Ninja Multi-Config" \
  -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
  -DDTTRSDK_DIR=.dttr/sdk

cmake --build build --config debug
```

On Windows using the MSYS2 MINGW32 shell:

```sh
cmake -S . -B build -G "Ninja Multi-Config" -DDTTRSDK_DIR=.dttr/sdk
cmake --build build --config debug
```

The compiled mod DLL is written to `build/debug/minimal-mod.dll`.

With Podman, build the template into a local container image, then copy the compiled DLL out of the artifact image:

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

The exported container-built DLL is written to `build/debug/minimal-mod.dll`, matching the local build output path.

## Optional Configuration

To use a different SDK path, pass it to CMake with `DTTRSDK_DIR`:

```sh
cmake -S . -B build -G "Ninja Multi-Config" -DDTTRSDK_DIR=/path/to/dttr/sdk
```

For container builds that skip fetching, `DTTRSDK_DIR` is also available as a build argument. The SDK path and required DttR module files must exist inside the container build context:

```sh
podman build -t dttr-minimal-mod -f Containerfile \
  --build-arg DTTR_FETCH_SDK=0 \
  --build-arg DTTRSDK_DIR=.dttr/sdk \
  .
```

To build against a different DttR release, edit `dttr-version.txt` and run the fetch script again.
