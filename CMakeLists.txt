cmake_minimum_required(VERSION 3.20)

set(CMAKE_CONFIGURATION_TYPES "debug;release" CACHE STRING "Available build configurations" FORCE)

project(dttr VERSION 0.0.1 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(dttrbuild)

if(NOT DEFINED DTTR_VERSION)
  set(DTTR_VERSION "${PROJECT_VERSION}")
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
            COMMAND git rev-parse --short HEAD
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE DTTR_GIT_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    if(DTTR_GIT_COMMIT)
      string(APPEND DTTR_VERSION "-${DTTR_GIT_COMMIT}")
    endif()
  endif()
endif()

option(DTTR_MODS_ENABLED "Enable modding/mod loading build" OFF)
option(TEST_DEPS_REQUIRED "Fail configure when optional test dependencies are missing" OFF)
option(PCDOGS_FIXTURES_REQUIRED "Fail fixture-backed tests instead of skipping when PCDOGS fixtures are missing" OFF)
set(PCDOGS_FIXTURES_DIR "gamefiles" CACHE PATH "Directory containing PCDOGS fixture executables for binary-backed tests")
include(CTest)
if(BUILD_TESTING)
  include(dttrtests)
  dttr_find_cmocka()
  add_custom_target(dttr_tests)
endif()

if(NOT WIN32)
  message(FATAL_ERROR "Windows only. Enter the Nix dev shell and use .toolchain/toolchain.cmake")
endif()

enable_language(RC)

dttr_configure_runtime_dependencies()
dttr_configure_ffmpeg()

dttr_configure_icon_assets()

add_subdirectory(vendor)
add_subdirectory(modules/common)
add_subdirectory(modules/sdk)
add_subdirectory(modules/mod_host)
add_subdirectory(modules/sidecar)
add_subdirectory(modules/loader)
add_subdirectory(modules/config_gui)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(DTTR_SUPPORT_STUB_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/stubs")
set(DTTR_SUPPORT_STUB_ASM "${DTTR_SUPPORT_STUB_GENERATED_DIR}/dttr_support_stub_entry.s")
set(DTTR_SUPPORT_STUB_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/generate-support-stubs.py")
set(DTTR_SUPPORT_STUB_OUTPUT_DIR "${CMAKE_BINARY_DIR}/stubs/$<CONFIG>")
set(
    DTTR_SUPPORT_STUB_NAMES
        mss32
        winstr
        winplay
)

set(DTTR_SUPPORT_STUB_DEFS ${DTTR_SUPPORT_STUB_NAMES})
list(TRANSFORM DTTR_SUPPORT_STUB_DEFS PREPEND "${DTTR_SUPPORT_STUB_GENERATED_DIR}/")
list(TRANSFORM DTTR_SUPPORT_STUB_DEFS APPEND ".def")

set(DTTR_SUPPORT_STUB_TARGETS ${DTTR_SUPPORT_STUB_NAMES})
list(TRANSFORM DTTR_SUPPORT_STUB_TARGETS PREPEND "dttr_stub_")

set(DTTR_SUPPORT_STUB_RUNTIME_DLLS ${DTTR_SUPPORT_STUB_TARGETS})
list(TRANSFORM DTTR_SUPPORT_STUB_RUNTIME_DLLS PREPEND "$<TARGET_FILE:")
list(TRANSFORM DTTR_SUPPORT_STUB_RUNTIME_DLLS APPEND ">")

set(DTTR_SUPPORT_STUB_SCRIPT_RUNNER
    "${CMAKE_COMMAND}" -E env
        PYTHONDONTWRITEBYTECODE=1
        "${Python3_EXECUTABLE}"
)
add_custom_command(
    OUTPUT ${DTTR_SUPPORT_STUB_ASM} ${DTTR_SUPPORT_STUB_DEFS}
    COMMAND ${DTTR_SUPPORT_STUB_SCRIPT_RUNNER}
        "${DTTR_SUPPORT_STUB_SCRIPT}"
        "${DTTR_SUPPORT_STUB_GENERATED_DIR}"
    DEPENDS
        "${DTTR_SUPPORT_STUB_SCRIPT}"
    COMMENT "Generating support DLL stub sources"
    VERBATIM
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
        RUNTIME_OUTPUT_DIRECTORY "${DTTR_SUPPORT_STUB_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${DTTR_SUPPORT_STUB_OUTPUT_DIR}"
    )

  target_link_options(${DTTR_SUPPORT_STUB_TARGET} PRIVATE
        -nostdlib
        -Wl,--entry,_DllMainCRTStartup@12
    )
endforeach()

set(DTTR_DIST_DIR "${CMAKE_BINARY_DIR}/dist/$<CONFIG>")
if(DTTR_MODS_ENABLED)
  string(APPEND DTTR_DIST_DIR "-modding")
endif()

set(DTTR_DIST_MODULES_DIR "${DTTR_DIST_DIR}/modules")

set(DTTR_DIST_MODULE_DLLS
    $<TARGET_FILE:dttr_launcher>
    $<TARGET_FILE:dttr_config>
    $<TARGET_FILE:dttr_sidecar>
    ${DTTR_SUPPORT_STUB_RUNTIME_DLLS}
    ${DTTR_SDL_RUNTIME_DLLS}
    ${DTTR_FFMPEG_RUNTIME_DLLS}
)

add_custom_target(dttr_dist ALL
    DEPENDS
        dttr
        dttr_config_exe
        dttr_config
        dttr_launcher
        dttr_sidecar
        ${DTTR_SUPPORT_STUB_TARGETS}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DTTR_DIST_MODULES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:dttr>
        "${DTTR_DIST_DIR}/dttr.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:dttr_config_exe>
        "${DTTR_DIST_DIR}/dttr-config.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DTTR_DIST_MODULE_DLLS}
        "${DTTR_DIST_MODULES_DIR}"
    COMMENT "Bundling runtime artifacts into ${DTTR_DIST_DIR}"
)

if(DTTR_MODS_ENABLED)
  dttr_copy_modding_sdk_distribution(dttr_dist "${DTTR_DIST_DIR}")
endif()
