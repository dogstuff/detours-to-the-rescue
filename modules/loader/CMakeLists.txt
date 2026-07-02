set(DTTR_LOADER_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(DTTR_LOADER_GENERATED_ASM_HEADER "${DTTR_LOADER_GENERATED_INCLUDE_DIR}/gen/asm.h")
set(DTTR_LOADER_GENERATED_PACKED_SDB_HEADER "${DTTR_LOADER_GENERATED_INCLUDE_DIR}/gen/packed_sdb.h")
set(DTTR_LOADER_GENERATED_HEADERS
    "${DTTR_LOADER_GENERATED_ASM_HEADER}"
    "${DTTR_LOADER_GENERATED_PACKED_SDB_HEADER}"
)
file(GLOB DTTR_LOADER_ASM_INPUTS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/asm/*.asm"
)

add_custom_command(
    OUTPUT "${DTTR_LOADER_GENERATED_ASM_HEADER}"
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate-asm-header.sh"
        "${CMAKE_CURRENT_SOURCE_DIR}/asm"
        "${DTTR_LOADER_GENERATED_ASM_HEADER}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate-asm-header.sh"
        ${DTTR_LOADER_ASM_INPUTS}
    COMMENT "Generating loader assembly header"
    VERBATIM
)

add_custom_command(
    OUTPUT "${DTTR_LOADER_GENERATED_PACKED_SDB_HEADER}"
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate-packed-sdb-header.sh"
        "${CMAKE_CURRENT_SOURCE_DIR}/compat/packed_sdb.bin"
        "${DTTR_LOADER_GENERATED_PACKED_SDB_HEADER}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate-packed-sdb-header.sh"
        "${CMAKE_CURRENT_SOURCE_DIR}/compat/packed_sdb.bin"
    COMMENT "Generating loader compatibility database header"
    VERBATIM
)

add_custom_target(dttr_loader_generated_headers
    DEPENDS ${DTTR_LOADER_GENERATED_HEADERS}
)

add_executable(dttr
    "${DTTR_ICON_RESOURCE}"
    src/main.c
)
set_source_files_properties("${DTTR_ICON_RESOURCE}" PROPERTIES
    OBJECT_DEPENDS "${DTTR_ICON_ICO}"
)
add_dependencies(dttr dttr_icon_assets)

dttr_use_windows_static_runtime(dttr WIN32_RELEASE)

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

target_include_directories(dttr_launcher PRIVATE
    "${DTTR_LOADER_GENERATED_INCLUDE_DIR}"
    include
)
add_dependencies(dttr_launcher dttr_loader_generated_headers)
dttr_set_target_version(dttr_launcher)
dttr_use_windows_static_runtime(dttr_launcher)

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
    ${DTTR_SDL_RUNTIME_DLLS}
    ${DTTR_FFMPEG_RUNTIME_DLLS}
)

add_dependencies(dttr dttr_launcher dttr_sidecar)
dttr_copy_runtime_files(dttr "${DTTR_LOADER_MODULES_DIR}"
    FILES
        ${DTTR_LOADER_MODULE_DLLS}
)
