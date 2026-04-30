find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_STATIC_LIBS OFF)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)

add_library(common STATIC
    src/log.c
    src/crashdump.c
    src/sdl.c
    src/config/defaults.c
    src/config/parse.c
    src/config/schema.c
    src/config/io.c
)
target_include_directories(common PUBLIC
    include
    ${SDL3_INCLUDE_DIRS}
)
target_compile_options(common PUBLIC
    ${SDL3_CFLAGS_OTHER}
)
target_link_libraries(common
    PUBLIC log
    PRIVATE sds klib dbghelp yyjson
)
target_compile_definitions(common PRIVATE
    DTTR_DEFAULT_LOG_LEVEL=$<IF:$<CONFIG:Debug>,LOG_DEBUG,LOG_INFO>
    DTTR_DEFAULT_MINIDUMP_TYPE=$<IF:$<CONFIG:Debug>,DTTR_MINIDUMP_DETAILED,DTTR_MINIDUMP_NORMAL>
)
