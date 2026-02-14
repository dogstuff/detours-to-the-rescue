find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_STATIC_LIBS OFF)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)

add_library(common STATIC
    src/crashdump.c
    src/config/defaults.c
    src/config/parse.c
    src/config/schema.c
    src/config/io.c
)
target_include_directories(common PUBLIC include)
target_link_libraries(common PUBLIC PkgConfig::SDL3)
target_link_libraries(common PRIVATE sds klib log dbghelp yyjson)
