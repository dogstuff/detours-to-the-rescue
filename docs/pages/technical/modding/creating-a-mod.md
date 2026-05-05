# Creating a Mod

DttR modding builds load component DLLs from `components/`. A DLL counts as a component only when it exports the two required entry points from [Component exports](exports.md). If either one is missing, DttR skips the DLL.

!!! warning

    The component API is experimental and can break between releases. Components should check `ctx->m_api_version` during initialization. `DTTR_COMPONENT_INIT` already does this.

## Minimal project

A DttR component is a 32-bit Windows DLL placed in the `components/` directory next to `dttr.exe`. Use a DttR **modding** build while developing components; normal builds do not load component DLLs.

### 1. Create a project directory

Create these directories and files:

- `my-dttr-component/`
- `my-dttr-component/CMakeLists.txt`
- `my-dttr-component/include/dttr_components.h`
- `my-dttr-component/src/component.c`

Copy or symlink `sdk/components/include/dttr_components.h` from this repository into `include/`.

### 2. Add `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_dttr_component LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(my_component SHARED
    src/component.c
)

target_include_directories(my_component PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(my_component PROPERTIES
    PREFIX ""
    OUTPUT_NAME "my_component"
)
```

The output DLL is `my_component.dll`. DttR uses the DLL filename in the configuration UI.

### 3. Add `src/component.c`

```c
#include <dttr_components.h>

static const DTTR_ComponentContext *g_ctx;

DTTR_COMPONENT_INFO("My Component", "0.1.0", "Your Name")

DTTR_COMPONENT_INIT {
    g_ctx = ctx;
    DTTR_COMPONENT_LOG_INFO(ctx, "my component loaded");
    return true;
}

DTTR_COMPONENT_CLEANUP {
    if (g_ctx) {
        DTTR_COMPONENT_LOG_INFO(g_ctx, "my component unloaded");
    }
    g_ctx = NULL;
}
```

Only `DTTR_COMPONENT_INIT` and `DTTR_COMPONENT_CLEANUP` are required. `DTTR_COMPONENT_INFO` is optional, but it makes the load obvious in the log.

### 4. Build a 32-bit Windows DLL

From the component project directory, configure CMake with a 32-bit Windows toolchain. For example, with MinGW-w64:

```sh
cmake -S . -B build \
  -G "Ninja Multi-Config" \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_SYSTEM_PROCESSOR=i686 \
  -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc
cmake --build build --config Release
```

The exact compiler path depends on your environment. The result must be a 32-bit Windows DLL, because the game process is 32-bit.

### 5. Install and run

1. Create `components/` next to `dttr.exe` if it does not already exist.
2. Copy `my_component.dll` into `components/`.
3. Run the DttR modding build.
4. Check `dttr.log` for `my component loaded`.

When hot reload is enabled, DttR copies component DLLs to temporary `_dttr_hot_...` shadow DLLs before loading them. That lets you rebuild the source DLL while the game is running; DttR reloads the changed DLL on the next component scan.
