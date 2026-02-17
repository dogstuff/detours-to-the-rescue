set(DTTR_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/gen")
set(DTTR_VLC_DIR "${CMAKE_SOURCE_DIR}/.toolchain/vlc")

add_library(dttr_sidecar SHARED
    src/entrypoint.c
    src/impl.c
    src/inputs/inputs.c
    src/inputs/hook_dinput_poll.c
    src/inputs/hook_getasynckeystate.c
    src/graphics/graphics.c
    src/graphics/frame.c
    src/graphics/resources.c
    src/graphics/pipelines.c
    src/graphics/util.c
    src/graphics/hooks.c
    src/graphics/com_direct3d7.c
    src/graphics/com_direct3ddevice7.c
    src/graphics/com_direct3dtexture2.c
    src/graphics/com_directdraw7.c
    src/graphics/com_directdrawsurface7.c
    src/other/hooks.c
    src/other/hook_crt_open_file.c
    src/other/hook_cleanup_level_assets.c
    src/other/hook_resolve_pcdogs_path.c
)

target_include_directories(dttr_sidecar PRIVATE
    include
    "${DTTR_SHADER_OUTPUT_DIR}"
    "${DTTR_VLC_DIR}/sdk/include"
)

target_link_options(dttr_sidecar PRIVATE -static)

target_compile_definitions(dttr_sidecar PRIVATE
    DTTR_VERSION="${DTTR_VERSION}"
)

add_library(vlc SHARED IMPORTED)
set_target_properties(vlc PROPERTIES
    IMPORTED_IMPLIB "${DTTR_VLC_DIR}/sdk/lib/libvlc.lib"
    IMPORTED_LOCATION "${DTTR_VLC_DIR}/libvlc.dll"
)

target_link_libraries(dttr_sidecar PRIVATE
    kernel32
    psapi

    vlc
    sds
    klib
    xxhash
    log
    common
)

add_custom_command(TARGET dttr_sidecar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/.toolchain/sdl3/i686-w64-mingw32/bin/SDL3.dll"
        "$<TARGET_FILE_DIR:dttr_sidecar>/SDL3.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DTTR_VLC_DIR}/libvlc.dll"
        "$<TARGET_FILE_DIR:dttr_sidecar>/libvlc.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DTTR_VLC_DIR}/libvlccore.dll"
        "$<TARGET_FILE_DIR:dttr_sidecar>/libvlccore.dll"
)
