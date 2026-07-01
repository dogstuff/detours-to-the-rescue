# Environment overrides
# - BUILD_DIR: Sets the directory for the CMake build output.
# - TEST_JOBS: Sets the parallel job count for build and test recipes.
# - SHADER_OUTPUT_DIR: Sets the directory for generated shader headers.
# - DTTR_VERSION: Sets the version string for package and build artifacts.
# - DTTR_MODS_ENABLED: Sets whether CMake configures modding support.
# - TEST_DEPS_REQUIRED: Sets whether missing test dependencies fail configuration.
# - PCDOGS_FIXTURES_REQUIRED: Sets whether missing PCDOGS fixtures fail configuration.
# - PCDOGS_FIXTURES_DIR: Sets the directory for PCDOGS fixture files.
# - DTTR_USE_CACHED_SDL3GPU_SHADERS: Sets whether cached SDL3 GPU shader artifacts are used.
# - DOCKER: Sets the Docker-compatible CLI used by container recipes.
# - DTTR_CONTAINER_PLATFORM: Sets the platform targeted by build containers.

build-dir := env_var_or_default("BUILD_DIR", "build")
build-config-debug := "debug"
build-config-release := "release"
test-jobs := env_var_or_default("TEST_JOBS", "8")

toolchain-dir := ".toolchain"
toolchain-file := ".toolchain/toolchain.cmake"

shader-output-dir := env_var_or_default("SHADER_OUTPUT_DIR", build-dir + "/modules/sidecar/generated/include/gen")
cached-sdl3gpu-shaders-dir := "modules/sidecar/shaders/cache/sdl3gpu"

docs-build-dir := "docs/build"
docs-config := "docs/zensical.toml"
docs-source-dir := build-dir + "/docs-source"
doxyfile := "docs/doxyfile.ini"

git-short-sha := `git rev-parse --short HEAD`
dttr-version := env_var_or_default("DTTR_VERSION", git-short-sha)
dttr-mods-enabled := env_var_or_default("DTTR_MODS_ENABLED", "OFF")

test-deps-required := env_var_or_default("TEST_DEPS_REQUIRED", "ON")
pcdogs-fixtures-required := env_var_or_default("PCDOGS_FIXTURES_REQUIRED", "ON")
pcdogs-fixtures-dir := env_var_or_default("PCDOGS_FIXTURES_DIR", "gamefiles")

host-cached-sdl3gpu-default := `test "$(uname -s)" = Darwin && printf ON || printf OFF`
use-cached-sdl3gpu-shaders := env_var_or_default("DTTR_USE_CACHED_SDL3GPU_SHADERS", host-cached-sdl3gpu-default)

docker := env_var_or_default("DOCKER", "podman")
container-image := "dttr-toolchain"
container-platform := env_var_or_default("DTTR_CONTAINER_PLATFORM", "linux/amd64")
build-containerfile := "build.Containerfile"

# Show available local build recipes.
_default:
    @just --list

# Configure the build; generated artifacts are produced by CMake in the build tree.
prepare-build: setup-build

# Default debug build.
build: prepare-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}"

# Optimized release build.
build-release: prepare-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-release }}"

# Keep the short test entrypoint aligned with the full suite.
test: test-all

# Full test pass for common, SDK, and sidecar.
test-all: ci-build-tests ci-test

# Build and run SDK-only tests.
test-sdk: setup-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}" --parallel "{{ test-jobs }}" --target dttr_sdk_tests
    ctest --test-dir "{{ build-dir }}" -C "{{ build-config-debug }}" --output-on-failure --parallel "{{ test-jobs }}" --no-tests=error -L "sdk"

# Build and run sidecar tests for runtime integration code.
test-sidecar: setup-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}" --parallel "{{ test-jobs }}" --target dttr_sidecar_tests
    ctest --test-dir "{{ build-dir }}" -C "{{ build-config-debug }}" --output-on-failure --parallel "{{ test-jobs }}" --no-tests=error -L "sidecar"

# Configure the cross-compiled Ninja build.
setup-build:
    cmake -G "Ninja Multi-Config" -B "{{ build-dir }}" \
      -DCMAKE_TOOLCHAIN_FILE="{{ toolchain-file }}" \
      -DDTTR_VERSION="{{ dttr-version }}" \
      -DDTTR_MODS_ENABLED={{ dttr-mods-enabled }} \
      -DTEST_DEPS_REQUIRED={{ test-deps-required }} \
      -DPCDOGS_FIXTURES_REQUIRED={{ pcdogs-fixtures-required }} \
      -DPCDOGS_FIXTURES_DIR="{{ pcdogs-fixtures-dir }}" \
      -DDTTR_USE_CACHED_SDL3GPU_SHADERS={{ use-cached-sdl3gpu-shaders }}

# Remove generated build and toolchain state.
clean:
    rm -rf "{{ build-dir }}" "{{ toolchain-dir }}"

# Compile sidecar shader assets into build-tree generated headers.
build-shaders:
    SHADER_OUTPUT_DIR="{{ shader-output-dir }}" just --justfile modules/sidecar/justfile build-shaders

# Regenerate SDK blueprint artifacts.
build-sdk-blueprints: setup-build
    just --justfile modules/sdk/justfile blueprints

# Regenerate the local SDL3 GPU shader cache.
update-cached-sdl3gpu-shaders:
    rm -rf "{{ cached-sdl3gpu-shaders-dir }}"
    bash ./modules/sidecar/scripts/build-shaders.sh "{{ cached-sdl3gpu-shaders-dir }}"

# Compile sidecar shaders inside the container.
build-shaders-container:
    SHADER_OUTPUT_DIR="{{ shader-output-dir }}" just --justfile modules/sidecar/justfile build-shaders-container

# Build the project in a container for stable artifacts.
build-container:
    {{ docker }} build --platform "{{ container-platform }}" -t "{{ container-image }}" -f "{{ build-containerfile }}" "{{ justfile_directory() }}"
    bash ./scripts/build-container.sh \
      "{{ justfile_directory() }}" \
      "{{ docker }}" \
      "{{ container-image }}" \
      "{{ container-platform }}" \
      "{{ toolchain-dir }}" \
      "build-container" \
      "{{ dttr-version }}" \
      "{{ git-short-sha }}" \
      {{ dttr-mods-enabled }}

# Build, archive, and optionally upload debug distributions.
package-debug version=dttr-version archive-id=git-short-sha package-registry-url="" job-token="":
    bash ./scripts/package-builds.sh "{{ version }}" "{{ archive-id }}" debug "{{ package-registry-url }}" "{{ job-token }}"

# Build, archive, and optionally upload release distributions.
package-release version=dttr-version archive-id=dttr-version package-registry-url="" job-token="":
    bash ./scripts/package-builds.sh "{{ version }}" "{{ archive-id }}" release "{{ package-registry-url }}" "{{ job-token }}"

# Format C sources and Python files.
format:
    find modules \
      -path '*/include/gen/*' -prune -o \
      -path '*/src/generated/*' -prune -o \
      \( -iname '*.h' -o -iname '*.c' \) -print | xargs clang-format -i
    git ls-files -z -- '*.py' ':!:vendor/**' | xargs -0 black

# Build the Zensical site and Doxygen output.
build-docs: setup-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}" --target dttr_pcdogs_generated_headers
    just ci-build-docs

# Build and serve the generated documentation site.
serve-docs port="3000": build-docs
    python3 -m http.server "{{ port }}" --bind 127.0.0.1 --directory "{{ docs-build-dir }}"

# Build the CTest binaries used by downstream CI test and docs jobs.
ci-build-tests: setup-build
    cmake --build "{{ build-dir }}" --config "{{ build-config-debug }}" --parallel "{{ test-jobs }}" --target dttr_tests

# Run common, SDK, and sidecar tests from an existing build tree.
ci-test:
    ctest --test-dir "{{ build-dir }}" -C "{{ build-config-debug }}" \
      --output-on-failure --parallel "{{ test-jobs }}" --no-tests=error \
      -L "common|sdk|sidecar"

# Build and check the SDL3 GPU shader cache for CI artifacts.
ci-build-shaders: update-cached-sdl3gpu-shaders
    #!/usr/bin/env bash
    set -euo pipefail
    cache_dir="{{ cached-sdl3gpu-shaders-dir }}"
    expected=(
      sdl3gpu_shaders.h
      shaders/basic.frag.dxil
      shaders/basic.frag.spv
      shaders/basic.vert.dxil
      shaders/basic.vert.spv
    )
    for artifact in "${expected[@]}"; do
      test -s "$cache_dir/$artifact"
    done

# Build documentation from an existing build for CI.
ci-build-docs:
    #!/usr/bin/env bash
    set -euo pipefail

    rm -rf "{{ docs-build-dir }}" "{{ docs-source-dir }}"

    python3 scripts/prepare-docs-source.py --config "{{ docs-config }}" --output-dir "{{ docs-source-dir }}"
    npm --prefix docs/symbol-viewer ci

    docs_js_out="{{ docs-source-dir }}/pages/assets/js/pcdogs-symbol-viewer.mjs"

    if [ "${docs_js_out#/}" = "$docs_js_out" ]; then
      docs_js_out="$(pwd)/$docs_js_out"
    fi

    mkdir -p "$(dirname "$docs_js_out")"
    npm --prefix docs/symbol-viewer run build -- --outfile="$docs_js_out"

    zensical build --clean --config-file "{{ docs-source-dir }}/zensical.toml"
    cp -R "{{ docs-source-dir }}/build" "{{ docs-build-dir }}"

    DTTR_SDK_GENERATED_INCLUDE_DIR="{{ build-dir }}/modules/sdk/generated/include" doxygen "{{ doxyfile }}"
