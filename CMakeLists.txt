cmake_minimum_required(VERSION 3.20)
project(dttr VERSION 0.0.1 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=)


if(NOT DEFINED DTTR_VERSION)
    set(DTTR_VERSION "${PROJECT_VERSION}")
endif()

option(DTTR_MODDING "Enable modding build with component loading system" OFF)

if(NOT WIN32)
    message(FATAL_ERROR
        "Windows only. Use: cmake -B build -DCMAKE_TOOLCHAIN_FILE=i686-w64-mingw32.cmake"
    )
endif()

add_subdirectory(vendor)
add_subdirectory(common)
add_subdirectory(sidecar)
add_subdirectory(loader)

set(DTTR_SUPPORT_STUB_MANIFEST "${CMAKE_SOURCE_DIR}/stubs/manifest.txt")
set(DTTR_SUPPORT_STUB_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/stubs")
set(DTTR_SUPPORT_STUB_ASM "${DTTR_SUPPORT_STUB_GENERATED_DIR}/dttr_support_stub_entry.s")
file(STRINGS "${DTTR_SUPPORT_STUB_MANIFEST}" DTTR_SUPPORT_STUB_NAMES REGEX [=[^\[[^]]+\]$]=])
list(TRANSFORM DTTR_SUPPORT_STUB_NAMES REPLACE [=[^\[([^]]+)\]$]=] [=[\1]=])

set(DTTR_SUPPORT_STUB_DEFS ${DTTR_SUPPORT_STUB_NAMES})
list(TRANSFORM DTTR_SUPPORT_STUB_DEFS PREPEND "${DTTR_SUPPORT_STUB_GENERATED_DIR}/")
list(TRANSFORM DTTR_SUPPORT_STUB_DEFS APPEND ".def")

set(DTTR_SUPPORT_STUB_TARGETS ${DTTR_SUPPORT_STUB_NAMES})
list(TRANSFORM DTTR_SUPPORT_STUB_TARGETS PREPEND "dttr_stub_")

set(DTTR_SUPPORT_STUB_RUNTIME_DLLS ${DTTR_SUPPORT_STUB_TARGETS})
list(TRANSFORM DTTR_SUPPORT_STUB_RUNTIME_DLLS PREPEND "$<TARGET_FILE:")
list(TRANSFORM DTTR_SUPPORT_STUB_RUNTIME_DLLS APPEND ">")

add_custom_command(
    OUTPUT ${DTTR_SUPPORT_STUB_ASM} ${DTTR_SUPPORT_STUB_DEFS}
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/generate-support-stubs.sh"
        "${DTTR_SUPPORT_STUB_MANIFEST}"
        "${DTTR_SUPPORT_STUB_GENERATED_DIR}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/scripts/generate-support-stubs.sh"
        "${DTTR_SUPPORT_STUB_MANIFEST}"
    COMMENT "Generating support DLL stub sources"
)

foreach(DTTR_SUPPORT_STUB_NAME DTTR_SUPPORT_STUB_TARGET IN ZIP_LISTS
    DTTR_SUPPORT_STUB_NAMES
    DTTR_SUPPORT_STUB_TARGETS
)
    add_library(${DTTR_SUPPORT_STUB_TARGET} SHARED
        "${DTTR_SUPPORT_STUB_ASM}"
        "${DTTR_SUPPORT_STUB_GENERATED_DIR}/${DTTR_SUPPORT_STUB_NAME}.def"
    )

    set_target_properties(${DTTR_SUPPORT_STUB_TARGET} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${DTTR_SUPPORT_STUB_NAME}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/stubs/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/stubs/$<CONFIG>"
    )

    target_link_options(${DTTR_SUPPORT_STUB_TARGET} PRIVATE
        -nostdlib
        -Wl,--entry,_DllMainCRTStartup@12
    )
endforeach()

set(DTTR_DIST_SUFFIX "")

if(DTTR_MODDING)
    set(DTTR_DIST_SUFFIX "-modding")
endif()

set(DTTR_DIST_DIR "${CMAKE_BINARY_DIR}/dist/$<CONFIG>${DTTR_DIST_SUFFIX}")
set(DTTR_DIST_MODULES_DIR "${DTTR_DIST_DIR}/modules")

set(DTTR_SUPPORT_STUB_STALE_ROOT_DLLS ${DTTR_SUPPORT_STUB_NAMES})
list(TRANSFORM DTTR_SUPPORT_STUB_STALE_ROOT_DLLS PREPEND "${DTTR_DIST_DIR}/")
list(TRANSFORM DTTR_SUPPORT_STUB_STALE_ROOT_DLLS APPEND ".dll")

set(DTTR_DIST_RUNTIME_DLLS
    ${DTTR_SUPPORT_STUB_RUNTIME_DLLS}
    "$<TARGET_FILE_DIR:dttr_sidecar>/SDL3.dll"
    "$<TARGET_FILE_DIR:dttr_sidecar>/SDL3_mixer.dll"
    ${DTTR_FFMPEG_RUNTIME_DLLS}
)

set(DTTR_DIST_MODULE_DLLS
    $<TARGET_FILE:dttr_launcher>
    $<TARGET_FILE:dttr_sidecar>
    ${DTTR_DIST_RUNTIME_DLLS}
)

set(DTTR_DIST_STALE_ROOT_DLLS
    "${DTTR_DIST_DIR}/libdttr_sidecar.dll"
    ${DTTR_SUPPORT_STUB_STALE_ROOT_DLLS}
    "${DTTR_DIST_DIR}/SDL3.dll"
    "${DTTR_DIST_DIR}/SDL3_mixer.dll"
)

foreach(DTTR_FFMPEG_RUNTIME_DLL IN LISTS DTTR_FFMPEG_RUNTIME_DLLS)
    get_filename_component(DTTR_FFMPEG_RUNTIME_DLL_NAME "${DTTR_FFMPEG_RUNTIME_DLL}" NAME)
    list(APPEND DTTR_DIST_STALE_ROOT_DLLS "${DTTR_DIST_DIR}/${DTTR_FFMPEG_RUNTIME_DLL_NAME}")
endforeach()

file(GENERATE
    OUTPUT "${DTTR_DIST_DIR}/dttr.jsonc"
    INPUT "${CMAKE_SOURCE_DIR}/common/dttr.jsonc"
)

add_custom_target(dttr_dist ALL
    DEPENDS dttr dttr_launcher dttr_sidecar ${DTTR_SUPPORT_STUB_TARGETS}
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DTTR_DIST_MODULES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DTTR_DIST_MODULES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -f ${DTTR_DIST_STALE_ROOT_DLLS}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:dttr> "${DTTR_DIST_DIR}/dttr.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DTTR_DIST_MODULE_DLLS} "${DTTR_DIST_MODULES_DIR}"
    COMMENT "Bundling runtime artifacts into ${DTTR_DIST_DIR}"
)

if(DTTR_MODDING)
    add_custom_command(TARGET dttr_dist POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${DTTR_DIST_DIR}/components"
            "${DTTR_DIST_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/components/include/dttr_components.h"
            "${DTTR_DIST_DIR}/include/dttr_components.h"
    )
endif()
