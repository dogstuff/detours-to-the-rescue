# DttR Minimal C Mod Template

A small cross-compilable C template project for writing [Detours to the Rescue](https://gitlab.com/dogstuff/detours-to-the-rescue) mods with MinGW.

## Quick Start

## 1. Get the Template

Download and extract the template archive from the latest DttR release:

[`dttr-mod-template-c.tar.gz`](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-mod-template-c.tar.gz)

## 2. Configure a Build Environment

Follow the steps for the machine you are building on:

<details>
<summary><strong>Nix (Linux/macOS/WSL)</strong></summary>

Nix users can simply enter the template's development shell to automatically install and configure the required toolchain:

```sh
nix develop
```

</details>

<details>
<summary><strong>APT-Based Linux Distributions (Ubuntu, Debian, etc.)</strong></summary>

Users of APT-based Linux distributions can install the required toolchain by running:

```sh
sudo apt update
sudo apt install cmake ninja-build curl unzip gcc-mingw-w64-i686
```

</details>

<details>
<summary><strong>RPM-Based Linux Distributions using DNF (Fedora, RHEL, etc.)</strong></summary>

Users of RPM-based Linux distributions with DNF can install the required toolchain by running:

```sh
sudo dnf install cmake ninja-build curl unzip mingw32-gcc
```

</details>

<details>
<summary><strong>macOS (Homebrew)</strong></summary>

Users of macOS with Homebrew can install the required build tools and MinGW compiler by running:

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

Then open an MSYS2 MINGW32 shell and install the required toolchain:

```sh
pacman -S --needed mingw-w64-i686-cmake mingw-w64-i686-ninja mingw-w64-i686-gcc curl unzip git
```

</details>

<details>
<summary><strong>Podman Container</strong></summary>

If you do not want to install the MinGW toolchain locally, install Podman and use the container build in step 4 instead. The container image installs the compiler and build tools for you.

```sh
podman --version
```

</details>

## 3. Fetch the DttR SDK

This project provides a convenient script to download the DttR release SDK corresponding to the version in `dttr-version.txt`.

If you are using the default Podman container build in step 4, you can skip this step; the container build runs the fetch script inside the image. If you pass `--build-arg DTTR_FETCH_SDK=0`, run this step first so the local SDK files exist in the build context.

On Linux, macOS, or inside the Nix shell:

```sh
./scripts/fetch-dttr.sh
```

On Windows (PowerShell):

```powershell
./scripts/fetch-dttr.ps1
```

To confirm the download was successful, the following file should now be present:

```
.dttr/sdk/DTTRSDKConfig.cmake
```

**NOTE: On some systems, the .dttr directory will only show visually if you have enabled showing hidden files.**

## 4. Build the DLL

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

The compiled mod DLL can then be found at `build/debug/minimal-mod.dll`.

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

The exported container-built DLL can then be found at `build/debug/minimal-mod.dll`, matching the local build output path.

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

To build against a different or newer DttR release, edit `dttr-version.txt` accordingly and run the fetch script again.
