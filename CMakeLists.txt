cmake_minimum_required(VERSION 3.20)
project(dttr VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=)

if(NOT DEFINED DTTR_VERSION)
    set(DTTR_VERSION "${PROJECT_VERSION}")
endif()

option(DTTR_MODDING "Enable modding build with component loading system" OFF)

if(NOT WIN32)
    message(FATAL_ERROR
        "Windows only. Use: cmake -B build -DCMAKE_TOOLCHAIN_FILE=cross/i686-w64-mingw32.cmake"
    )
endif()

add_subdirectory(vendor)
add_subdirectory(common)
add_subdirectory(sidecar)
add_subdirectory(loader)

set(DTTR_DIST_SUFFIX "")
if(DTTR_MODDING)
    set(DTTR_DIST_SUFFIX "-modding")
endif()

set(DTTR_DIST_DIR "${CMAKE_BINARY_DIR}/dist/$<CONFIG>${DTTR_DIST_SUFFIX}")
set(DTTR_DIST_MODULES_DIR "${DTTR_DIST_DIR}/modules")
file(GENERATE
    OUTPUT "${DTTR_DIST_DIR}/dttr.jsonc"
    INPUT "${CMAKE_SOURCE_DIR}/common/dttr.jsonc"
)

add_custom_target(dttr_dist ALL
    DEPENDS dttr dttr_sidecar
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DTTR_DIST_MODULES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${DTTR_DIST_DIR}/*.dll"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${DTTR_DIST_MODULES_DIR}/libmpv-2.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:dttr> "${DTTR_DIST_DIR}/dttr.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:dttr_sidecar> "${DTTR_DIST_MODULES_DIR}/libdttr_sidecar.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:dttr_sidecar>/SDL3.dll" "${DTTR_DIST_MODULES_DIR}/SDL3.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DTTR_FFMPEG_RUNTIME_DLLS} "${DTTR_DIST_MODULES_DIR}"
    COMMENT "Bundling runtime artifacts into ${DTTR_DIST_DIR}"
)

if(DTTR_MODDING)
    add_custom_command(TARGET dttr_dist POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DTTR_DIST_DIR}/components"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DTTR_DIST_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/components/include/dttr_components.h"
            "${DTTR_DIST_DIR}/include/dttr_components.h"
    )
endif()
