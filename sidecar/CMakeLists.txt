set(DTTR_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/gen")
set(DTTR_MPV_DIR "${CMAKE_SOURCE_DIR}/.toolchain/mpv")

add_library(dttr_sidecar SHARED
    src/entrypoint.c
    src/impl.c
    src/game_api.c
    src/hook_registry.c
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
    src/other/hooks.c
    src/other/hook_crt_open_file.c
    src/other/hook_cleanup_level_assets.c
    src/other/hook_resolve_pcdogs_path.c
    src/movies/movies.c
)

target_include_directories(dttr_sidecar PRIVATE
    include
    "${CMAKE_SOURCE_DIR}/components/include"
    "${DTTR_SHADER_OUTPUT_DIR}"
    "${DTTR_MPV_DIR}/include"
)

target_link_options(dttr_sidecar PRIVATE -static)

target_compile_definitions(dttr_sidecar PRIVATE
    DTTR_VERSION="${DTTR_VERSION}"
)

add_library(mpv SHARED IMPORTED)
set_target_properties(mpv PROPERTIES
    IMPORTED_IMPLIB "${DTTR_MPV_DIR}/libmpv.dll.a"
    IMPORTED_LOCATION "${DTTR_MPV_DIR}/libmpv-2.dll"
)

target_link_libraries(dttr_sidecar PRIVATE
    kernel32
    psapi
    opengl32

    mpv
    Zydis
    sds
    klib
    xxhash
    log
    common
)

if(DTTR_COMPONENTS)
    target_sources(dttr_sidecar PRIVATE
        src/components/components.c
        src/graphics/overlay_components.c
    )
    target_compile_definitions(dttr_sidecar PRIVATE DTTR_COMPONENTS_ENABLED)
endif()

add_custom_command(TARGET dttr_sidecar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/.toolchain/sdl3/i686-w64-mingw32/bin/SDL3.dll"
        "$<TARGET_FILE_DIR:dttr_sidecar>/SDL3.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DTTR_MPV_DIR}/libmpv-2.dll"
        "$<TARGET_FILE_DIR:dttr_sidecar>/libmpv-2.dll"
)
