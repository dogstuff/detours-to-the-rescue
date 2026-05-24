build-dir := env_var_or_default("BUILD_DIR", "build")
build-config-debug := "Debug"
build-config-release := "Release"
test-max-parallel := env_var_or_default("TEST_JOBS", "8")
toolchain-dir := ".toolchain"
toolchain-file := ".toolchain/toolchain.cmake"
shader-output-dir := env_var_or_default("SHADER_OUTPUT_DIR", build-dir + "/modules/sidecar/generated/include/gen")
cached-sdl3gpu-shader-dir := "modules/sidecar/shaders/cache/sdl3gpu"
format-dirs := "./modules/loader ./modules/sidecar ./modules/common ./modules/sdk"
docs-build-dir := "docs/build"
docs-config := "docs/zensical.toml"
doxyfile := "docs/doxyfile.ini"
git-short-sha := `git rev-parse --short HEAD`
dttr-version := env_var_or_default("DTTR_VERSION", git-short-sha)
dttr-modding := env_var_or_default("DTTR_MODS_ENABLED", "OFF")
require-test-deps := env_var_or_default("DTTR_REQUIRE_TEST_DEPS", "OFF")
require-pcdogs-fixtures := env_var_or_default("DTTR_REQUIRE_PCDOGS_FIXTURES", "OFF")
pcdogs-fixture-dir := env_var_or_default("DTTR_PCDOGS_FIXTURE_DIR", "fixture")
host-cached-sdl3gpu-default := `test "$(uname -s)" = Darwin && printf ON || printf OFF`
use-cached-sdl3gpu-shaders := env_var_or_default("DTTR_USE_CACHED_SDL3GPU_SHADERS", host-cached-sdl3gpu-default)
docker := "podman"
container-image := "dttr-toolchain"
container-platform := env_var_or_default("DTTR_CONTAINER_PLATFORM", "linux/amd64")
containerfile-build := "build.Containerfile"

# Show available local build recipes.
_default:
    @just --list

# Configure the build; generated artifacts are produced by CMake in the build tree.
prepare-build: setup-build

# Build the default debug distribution.
build: prepare-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}"

# Build the release distribution.
build-release: prepare-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-release }}"

# Keep the short test entrypoint aligned with the full suite.
test: test-all

# Build all first-party test executables for CI.
ci-compile-tests: setup-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}" --parallel "{{ test-max-parallel }}" --target dttr_tests

# Build the CI build tree shared by downstream test and docs jobs.
ci-build-internal: ci-compile-tests

# Run common, SDK, and sidecar tests from an existing build tree.
ci-test:
    ctest --test-dir "{{ build-dir }}" -C "{{ build-config-debug }}" \
      --output-on-failure --parallel "{{ test-max-parallel }}" -L "common|sdk|sidecar"

# Build and run common, SDK, and sidecar tests.
test-all: ci-compile-tests ci-test

# Build and run SDK-only tests.
test-sdk: setup-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}" --parallel "{{ test-max-parallel }}" --target dttr_sdk_tests
    ctest --test-dir "{{ build-dir }}" -C "{{ build-config-debug }}" --output-on-failure --parallel "{{ test-max-parallel }}" -L "sdk"

# Build and run sidecar tests for runtime integration code.
test-sidecar: setup-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}" --parallel "{{ test-max-parallel }}" --target dttr_sidecar_tests
    ctest --test-dir "{{ build-dir }}" -C "{{ build-config-debug }}" --output-on-failure --parallel "{{ test-max-parallel }}" -L "sidecar"

# Configure the cross-compiled Ninja build.
setup-build:
    cmake -G "Ninja Multi-Config" -B "{{ build-dir }}" \
      -DCMAKE_TOOLCHAIN_FILE="{{ toolchain-file }}" \
      -DDTTR_VERSION="{{ dttr-version }}" \
      -DDTTR_MODS_ENABLED={{ dttr-modding }} \
      -DDTTR_REQUIRE_TEST_DEPS={{ require-test-deps }} \
      -DDTTR_REQUIRE_PCDOGS_FIXTURES={{ require-pcdogs-fixtures }} \
      -DDTTR_PCDOGS_FIXTURE_DIR="{{ pcdogs-fixture-dir }}" \
      -DDTTR_USE_CACHED_SDL3GPU_SHADERS={{ use-cached-sdl3gpu-shaders }}

# Remove generated build and toolchain state.
clean:
    rm -rf "{{ build-dir }}" "{{ toolchain-dir }}"

# Regenerate loader assembly outputs in the build tree.
_build-asm:
    just --justfile modules/loader/justfile asm

# Regenerate loader compatibility outputs in the build tree.
_build-compat:
    just --justfile modules/loader/justfile compat

# Compile sidecar shader assets into build-tree generated headers.
build-shaders:
    SHADER_OUTPUT_DIR="{{ shader-output-dir }}" just --justfile modules/sidecar/justfile build-shaders

# Regenerate SDK blueprint artifacts.
build-sdk-blueprints: setup-build
    just --justfile modules/sdk/justfile blueprints

# Regenerate the local SDL3 GPU shader cache.
update-cached-sdl3gpu-shaders:
    rm -rf "{{ cached-sdl3gpu-shader-dir }}"
    bash ./modules/sidecar/scripts/build-shaders.sh "{{ cached-sdl3gpu-shader-dir }}"

# Build and check the SDL3 GPU shader cache for CI artifacts.
ci-shader-build: update-cached-sdl3gpu-shaders
    #!/usr/bin/env bash
    set -euo pipefail
    cache_dir="{{ cached-sdl3gpu-shader-dir }}"
    expected=(
      sdl3gpu_shaders.h
      shaders/basic.frag.dxil
      shaders/basic.frag.spv
      shaders/basic.vert.dxil
      shaders/basic.vert.spv
      shaders/buf2tex.comp.dxil
      shaders/buf2tex.comp.spv
    )
    for artifact in "${expected[@]}"; do
      test -s "$cache_dir/$artifact"
    done

# Compile sidecar shaders inside the container.
build-shaders-container:
    SHADER_OUTPUT_DIR="{{ shader-output-dir }}" just --justfile modules/sidecar/justfile build-shaders-container

# Build the project in a container for stable artifacts.
build-container:
    {{ docker }} build --platform "{{ container-platform }}" -t "{{ container-image }}" -f "{{ containerfile-build }}" "{{ justfile_directory() }}"
    bash ./scripts/build-container.sh \
      "{{ justfile_directory() }}" \
      "{{ docker }}" \
      "{{ container-image }}" \
      "{{ container-platform }}" \
      "{{ toolchain-dir }}" \
      "build-container" \
      "{{ dttr-version }}" \
      "{{ git-short-sha }}" \
      {{ dttr-modding }}

# Build and archive debug distributions.
package-debug version=dttr-version archive-id=git-short-sha:
    bash ./scripts/package-builds.sh "{{ version }}" "{{ archive-id }}" debug

# Build, archive, and optionally upload release distributions.
package-release version=dttr-version archive-id=dttr-version package-registry-url="" job-token="":
    bash ./scripts/package-builds.sh "{{ version }}" "{{ archive-id }}" release "{{ package-registry-url }}" "{{ job-token }}"

# Format C sources and Python files.
format:
    echo "Running formatter"
    find {{ format-dirs }} \
      -path '*/include/gen/*' -prune -o \
      -path '*/src/generated/*' -prune -o \
      \( -iname '*.h' -o -iname '*.c' \) -print | xargs clang-format -i
    black . --extend-exclude '^/vendor/'

# Build the Zensical site and Doxygen output.
build-docs: setup-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}" --target dttr_pcdogs_generated_headers
    just build-docs-from-build

# Build documentation from an existing CI build tree.
build-docs-from-build:
    test -s "{{ build-dir }}/modules/sdk/generated/include/dttr_pcdogs.h"
    test -s "{{ build-dir }}/modules/sdk/generated/include/dttr_pcdogs_unstable.h"
    rm -rf "{{ docs-build-dir }}"
    bash ./scripts/update-latest-release-link.sh
    zensical build --clean --config-file "{{ docs-config }}"
    DTTR_SDK_GENERATED_INCLUDE_DIR="{{ build-dir }}/modules/sdk/generated/include" doxygen "docs/doxyfile-sdk.ini"
    doxygen "{{ doxyfile }}"
    test ! -s "{{ docs-build-dir }}/doxygen-sdk-warnings.log"
    test ! -s "{{ docs-build-dir }}/doxygen-internal-warnings.log"

# Build and serve the generated documentation site.
serve-docs port="3000": build-docs
    python3 -m http.server "{{ port }}" --bind 127.0.0.1 --directory "{{ docs-build-dir }}"
