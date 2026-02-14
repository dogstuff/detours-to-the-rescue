cmake_minimum_required(VERSION 3.20)
project(dttr VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=)

if(NOT DEFINED DTTR_VERSION)
    set(DTTR_VERSION "${PROJECT_VERSION}")
endif()

if(NOT WIN32)
    message(FATAL_ERROR "Windows only. Use: cmake -B build -DCMAKE_TOOLCHAIN_FILE=cross/i686-w64-mingw32.cmake")
endif()

add_subdirectory(vendor)
add_subdirectory(common)
add_subdirectory(sidecar)
add_subdirectory(loader)

set(DTTR_DIST_DIR "${CMAKE_BINARY_DIR}/dist/$<CONFIG>")
set(DTTR_DEFAULT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/common/dttr.jsonc")

add_custom_target(dttr_dist ALL
    DEPENDS dttr dttr_sidecar
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DTTR_DIST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:dttr> "${DTTR_DIST_DIR}/dttr.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:dttr_sidecar> "${DTTR_DIST_DIR}/libdttr_sidecar.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:dttr_sidecar>/SDL3.dll" "${DTTR_DIST_DIR}/SDL3.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:dttr_sidecar>/libvlc.dll" "${DTTR_DIST_DIR}/libvlc.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:dttr_sidecar>/libvlccore.dll" "${DTTR_DIST_DIR}/libvlccore.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DTTR_DEFAULT_CONFIG_FILE}" "${DTTR_DIST_DIR}/dttr.jsonc"
    COMMENT "Bundling runtime artifacts into ${DTTR_DIST_DIR}"
)
