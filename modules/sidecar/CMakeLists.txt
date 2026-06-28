# Sidecar links SDL, FFmpeg, hook targets, and generated PCDogs headers into the injected DLL.
dttr_find_sdl3()
dttr_find_sdl3_mixer()

set(DTTR_SIDECAR_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(DTTR_SIDECAR_GENERATED_SHADER_DIR "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen")
set(DTTR_SIDECAR_OPENGL_SHADER_HEADER "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/opengl_shaders.h")
set(DTTR_SIDECAR_SDL3GPU_SHADER_HEADER "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/sdl3gpu_shaders.h")
set(DTTR_SIDECAR_SDL3GPU_SHADER_OUTPUTS
    "${DTTR_SIDECAR_SDL3GPU_SHADER_HEADER}"
    "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/shaders/basic.frag.dxil"
    "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/shaders/basic.frag.spv"
    "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/shaders/basic.vert.dxil"
    "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/shaders/basic.vert.spv"
)
set(DTTR_SIDECAR_GENERATED_SHADER_OUTPUTS
    "${DTTR_SIDECAR_OPENGL_SHADER_HEADER}"
    ${DTTR_SIDECAR_SDL3GPU_SHADER_OUTPUTS}
)

set(DTTR_USE_CACHED_SDL3GPU_SHADERS_DEFAULT OFF)
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(DTTR_USE_CACHED_SDL3GPU_SHADERS_DEFAULT ON)
endif()

option(
    DTTR_USE_CACHED_SDL3GPU_SHADERS
    "Copy cached SDL3 GPU shader blobs instead of compiling them with host shader tools"
    ${DTTR_USE_CACHED_SDL3GPU_SHADERS_DEFAULT}
)
set(DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders/cache/sdl3gpu")
set(DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_INPUTS
    "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/sdl3gpu_shaders.h"
    "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/shaders/basic.frag.dxil"
    "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/shaders/basic.frag.spv"
    "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/shaders/basic.vert.dxil"
    "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/shaders/basic.vert.spv"
)
file(GLOB DTTR_SIDECAR_OPENGL_SHADER_INPUTS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/opengl/*.glsl"
)
file(GLOB DTTR_SIDECAR_SDL3GPU_SHADER_INPUTS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/sdl3gpu/*.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/sdl3gpu/*.spv"
)

add_custom_command(
    OUTPUT "${DTTR_SIDECAR_OPENGL_SHADER_HEADER}"
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-opengl-shaders.sh"
        "${DTTR_SIDECAR_GENERATED_SHADER_DIR}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-opengl-shaders.sh"
        ${DTTR_SIDECAR_OPENGL_SHADER_INPUTS}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating sidecar OpenGL shader header"
    VERBATIM
)

if(DTTR_USE_CACHED_SDL3GPU_SHADERS)
    foreach(DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_INPUT IN LISTS DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_INPUTS)
        if(NOT EXISTS "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_INPUT}")
            message(FATAL_ERROR "Missing cached SDL3 GPU shader artifact: ${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_INPUT}")
        endif()
    endforeach()

    add_custom_command(
        OUTPUT ${DTTR_SIDECAR_SDL3GPU_SHADER_OUTPUTS}
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${DTTR_SIDECAR_GENERATED_SHADER_DIR}"
            "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/shaders"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/sdl3gpu_shaders.h"
            "${DTTR_SIDECAR_SDL3GPU_SHADER_HEADER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/shaders/basic.frag.dxil"
            "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/shaders/basic.frag.dxil"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/shaders/basic.frag.spv"
            "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/shaders/basic.frag.spv"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/shaders/basic.vert.dxil"
            "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/shaders/basic.vert.dxil"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_DIR}/shaders/basic.vert.spv"
            "${DTTR_SIDECAR_GENERATED_SHADER_DIR}/shaders/basic.vert.spv"
        DEPENDS ${DTTR_SIDECAR_CACHED_SDL3GPU_SHADER_INPUTS}
        COMMENT "Copying cached sidecar SDL3 GPU shader headers and blobs"
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT ${DTTR_SIDECAR_SDL3GPU_SHADER_OUTPUTS}
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-shaders.sh"
            "${DTTR_SIDECAR_GENERATED_SHADER_DIR}"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-shaders.sh"
            ${DTTR_SIDECAR_SDL3GPU_SHADER_INPUTS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating sidecar SDL3 GPU shader headers and blobs"
        VERBATIM
    )
endif()

add_custom_target(dttr_sidecar_generated_shaders
    DEPENDS ${DTTR_SIDECAR_GENERATED_SHADER_OUTPUTS}
)

set(DTTR_MSS_SOURCES
    src/audio/mss_core.c
    src/audio/mss_sample.c
    src/audio/mss_sdl.c
    src/audio/mss_stream.c
    src/audio/mss_track.c
    src/audio/mss_wave.c
)

add_library(dttr_sidecar SHARED
    src/entrypoint.c
    src/bootstrap.c
    src/context.c
    src/crash.c
    src/events.c

    src/inputs/inputs.c
    src/inputs/byte_patches.c
    src/inputs/controls_menu.c
    src/inputs/hook_dinput_poll.c
    src/inputs/hook_getasynckeystate.c
    src/inputs/hook_key_state.c
    src/inputs/hook_get_pressed_button.c
    src/inputs/hook_read_gamepad.c
    src/inputs/hook_rumble.c

    src/graphics/graphics.c
    src/graphics/util.c
    src/graphics/hooks.c
    src/graphics/backend_sdl3gpu.c
    src/graphics/backend_sdl3gpu_pipelines.c
    src/graphics/backend_sdl3gpu_resources.c
    src/graphics/backend_opengl.c
    src/graphics/com_direct3d7.c
    src/graphics/com_direct3ddevice7.c
    src/graphics/com_direct3dtexture2.c
    src/graphics/com_directdraw7.c
    src/graphics/com_directdrawsurface7.c

    src/audio/hooks.c
    ${DTTR_MSS_SOURCES}

    src/game/hooks.c
    src/game_data.c
    src/game/hook_crt_open_file.c
    src/game/hook_cleanup_title_resources.c
    src/game/hook_resolve_pcdogs_path.c

    src/movies/movies.c

    "${CMAKE_SOURCE_DIR}/modules/sdk/src/util_pkg_walk.c"
)

add_dependencies(dttr_sidecar dttr_pcdogs_generated_headers dttr_sidecar_generated_shaders)

target_compile_definitions(dttr_sidecar PRIVATE DTTR_SDK_ENABLE_UNSTABLE)

target_include_directories(dttr_sidecar PRIVATE
    "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}"
    include
    src
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/modules/sdk/include"
    "${DTTR_FFMPEG_DIR}/include"
)

dttr_use_windows_static_runtime(dttr_sidecar)
dttr_set_target_version(dttr_sidecar)

target_link_libraries(dttr_sidecar PRIVATE
    kernel32
    dbghelp
    PkgConfig::SDL3
    PkgConfig::SDL3_MIXER
    ${DTTR_FFMPEG_TARGETS}
    glad
    dttr_pcdogs_signatures
    dttr_sdk_runtime
    klib
    xxhash
    dr_wav
    common
)

if(DTTR_MODS_ENABLED)
    target_sources(dttr_sidecar PRIVATE
        src/timing.c
        src/game/frame_pacing.c
        src/mods/mods.c
        src/graphics/imgui_overlay.c
    )

    target_compile_definitions(dttr_sidecar PRIVATE DTTR_MODS_ENABLED)
    target_link_libraries(dttr_sidecar PRIVATE cimgui_static stdc++)
    target_link_options(dttr_sidecar PRIVATE -Wl,--export-all-symbols)
endif()

dttr_copy_runtime_files(dttr_sidecar "$<TARGET_FILE_DIR:dttr_sidecar>"
    FILES
        ${DTTR_SDL_RUNTIME_DLLS}
        ${DTTR_FFMPEG_RUNTIME_DLLS}
)

if(BUILD_TESTING)
    include("${CMAKE_CURRENT_LIST_DIR}/tests/tests.cmake")
endif()
