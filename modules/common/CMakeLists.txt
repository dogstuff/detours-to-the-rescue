find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_STATIC_LIBS OFF)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)

add_library(common STATIC
    src/log.c
    src/errors.c
    src/crashdump.c
    src/imgui_dialog.c
    src/imgui_error_ui.c
    src/path.c
    src/sdl.c
    src/iso/iso.c
    src/config/defaults.c
    src/config/io.c
    src/config/parse.c
    src/config/schema.c
)
target_include_directories(common PUBLIC
    include
    ${SDL3_INCLUDE_DIRS}
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
        dbghelp
        glad
        klib
        physfs-static
        yyjson
)
target_compile_definitions(common PRIVATE
    DTTR_DEFAULT_LOG_LEVEL=$<IF:$<CONFIG:Debug>,LOG_DEBUG,LOG_INFO>
    DTTR_DEFAULT_MINIDUMP_TYPE=$<IF:$<CONFIG:Debug>,DTTR_MINIDUMP_DETAILED,DTTR_MINIDUMP_NORMAL>
    DTTR_VERSION="${DTTR_VERSION}"
)
