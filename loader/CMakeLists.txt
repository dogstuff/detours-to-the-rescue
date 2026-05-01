add_executable(dttr
    src/main.c
)

target_link_options(dttr PRIVATE
    -static
    $<$<CONFIG:Release>:-mwindows>
)

target_link_libraries(dttr PRIVATE
    kernel32
    user32
)

add_library(dttr_launcher SHARED
    src/launcher/launcher.c
    src/launcher/browse.c
    src/launcher/load.c
    src/launcher/nt_process.c
    src/launcher/paths.c
    src/launcher/ui.c
    src/launcher/ui_logic.c
    src/launcher/watchdog.c
)

target_include_directories(dttr_launcher PRIVATE include)
target_compile_definitions(dttr_launcher PRIVATE
    DTTR_VERSION="${DTTR_VERSION}"
)
target_link_options(dttr_launcher PRIVATE -static)

target_link_libraries(dttr_launcher PRIVATE
    kernel32
    user32
    psapi
    opengl32
    dbghelp
    sds
    xxhash
    log
    common
)

set(DTTR_LOADER_MODULES_DIR "$<TARGET_FILE_DIR:dttr>/modules")
set(DTTR_LOADER_MODULE_DLLS
    $<TARGET_FILE:dttr_launcher>
    $<TARGET_FILE:dttr_sidecar>
    "$<TARGET_FILE_DIR:dttr_sidecar>/SDL3.dll"
    "$<TARGET_FILE_DIR:dttr_sidecar>/SDL3_mixer.dll"
    ${DTTR_FFMPEG_RUNTIME_DLLS}
)

add_dependencies(dttr dttr_launcher dttr_sidecar)
add_custom_command(TARGET dttr POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DTTR_LOADER_MODULES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DTTR_LOADER_MODULE_DLLS}
        "${DTTR_LOADER_MODULES_DIR}"
)
