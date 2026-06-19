# Builds shared runtime helpers used by loader, config UI, and tests.
dttr_find_sdl3()

add_library(common STATIC
    src/log.c
    src/errors.c
    src/crashdump.c
    src/imgui_dialog.c
    src/imgui_error_ui.c
    src/path.c
    src/gamepad_mapping.c
    src/input_binding.c
    src/sdl.c
    src/iso/iso.c
    src/config/defaults.c
    src/config/io.c
    src/config/mods.c
    src/config/parse.c
    src/config/schema.c
)
target_include_directories(common PUBLIC
    include
    "${CMAKE_SOURCE_DIR}/modules/sdk/include"
    ${SDL3_INCLUDE_DIRS}
)
target_include_directories(common PRIVATE
    "${CMAKE_BINARY_DIR}/modules/sdk/generated/include"
)
target_compile_options(common PUBLIC
    ${SDL3_CFLAGS_OTHER}
)
target_link_libraries(common
    PUBLIC
        cimgui_static
        log
        PkgConfig::SDL3
        sds
    PRIVATE
        cwalk
        dbghelp
        glad
        klib
        physfs-static
        Zydis
        yyjson
)
target_compile_definitions(common PRIVATE
    DTTR_DEFAULT_LOG_LEVEL=$<IF:$<CONFIG:debug>,LOG_DEBUG,LOG_INFO>
    DTTR_DEFAULT_MINIDUMP_TYPE=$<IF:$<CONFIG:debug>,DTTR_MINIDUMP_DETAILED,DTTR_MINIDUMP_NORMAL>
    DTTR_VERSION="${DTTR_VERSION}"
)

if(BUILD_TESTING)
    include("${CMAKE_CURRENT_LIST_DIR}/tests/tests.cmake")
endif()
