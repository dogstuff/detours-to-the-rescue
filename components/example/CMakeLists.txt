cmake_minimum_required(VERSION 3.20)
project(example_component LANGUAGES C)

set(CMAKE_C_STANDARD 17)

include(FetchContent)
FetchContent_Declare(dttr_components
    GIT_REPOSITORY https://gitlab.com/dogstuff/detours-to-the-rescue.git
    GIT_TAG        main
    SOURCE_SUBDIR  components
)
FetchContent_MakeAvailable(dttr_components)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)

set(DTTR_MPV_DIR "${CMAKE_SOURCE_DIR}/.toolchain/mpv")

add_library(example_component SHARED example_component.c)

target_include_directories(example_component PRIVATE
    "${DTTR_COMPONENTS_SOURCE_DIR}/include"
    "${DTTR_MPV_DIR}/include"
)

target_link_libraries(example_component PRIVATE PkgConfig::SDL3)

target_link_options(example_component PRIVATE -static)
