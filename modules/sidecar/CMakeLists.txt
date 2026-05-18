# Sidecar links SDL, FFmpeg, hook targets, and generated PCDogs headers into the injected DLL.
dttr_find_sdl3()
dttr_find_sdl3_mixer()

set(DTTR_SIDECAR_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(DTTR_SIDECAR_OPENGL_SHADER_HEADER "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/opengl_shaders.h")
set(DTTR_SIDECAR_SDL3GPU_SHADER_HEADER "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/sdl3gpu_shaders.h")
set(DTTR_SIDECAR_GENERATED_SHADER_OUTPUTS
    "${DTTR_SIDECAR_OPENGL_SHADER_HEADER}"
    "${DTTR_SIDECAR_SDL3GPU_SHADER_HEADER}"
    "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/basic.frag.dxil"
    "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/basic.frag.spv"
    "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/basic.vert.dxil"
    "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/basic.vert.spv"
    "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/buf2tex.comp.dxil"
    "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/buf2tex.comp.spv"
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
        "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-opengl-shaders.sh"
        ${DTTR_SIDECAR_OPENGL_SHADER_INPUTS}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating sidecar OpenGL shader header"
    VERBATIM
)

add_custom_command(
    OUTPUT
        "${DTTR_SIDECAR_SDL3GPU_SHADER_HEADER}"
        "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/basic.frag.dxil"
        "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/basic.frag.spv"
        "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/basic.vert.dxil"
        "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/basic.vert.spv"
        "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/buf2tex.comp.dxil"
        "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen/shaders/buf2tex.comp.spv"
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-shaders.sh"
        "${DTTR_SIDECAR_GENERATED_INCLUDE_DIR}/gen"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-shaders.sh"
        ${DTTR_SIDECAR_SDL3GPU_SHADER_INPUTS}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating sidecar SDL3 GPU shader headers and blobs"
    VERBATIM
)

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
    src/impl.c

    src/inputs/inputs.c
    src/inputs/hook_dinput_poll.c
    src/inputs/hook_getasynckeystate.c

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
    include("${CMAKE_CURRENT_LIST_DIR}/test/tests.cmake")
endif()
