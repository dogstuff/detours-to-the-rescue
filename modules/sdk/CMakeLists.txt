set(DTTR_PCDOGS_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_headers.py")
set(DTTR_PCDOGS_GENERATOR_INPUTS
    "${DTTR_PCDOGS_GENERATOR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/codegen.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/blueprint.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/header_template.h.mako"
    "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/pcdogs_docs.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/dttr_pcdogs.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/dttr_pcdogs_unstable.py"
)
set(DTTR_SDK_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(DTTR_SDK_GENERATED_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/src")
set(DTTR_SDK_GENERATED_INCLUDE_DIR "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    CACHE INTERNAL "Generated DttR SDK include directory"
)
set(DTTR_PCDOGS_GENERATED_PUBLIC_OUTPUTS
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_pcdogs.h"
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_pcdogs_unstable.h"
)
set(DTTR_SDK_STABLE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_runtime.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_core.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_mods.h"
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_pcdogs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_util.h"
)
set(DTTR_PCDOGS_GENERATED_PRIVATE_OUTPUTS
    "${DTTR_SDK_GENERATED_SRC_DIR}/pcdogs.c"
    "${DTTR_SDK_GENERATED_SRC_DIR}/generated/dttr_pcdogs_full.h"
    "${DTTR_SDK_GENERATED_SRC_DIR}/generated/dttr_pcdogs_unstable_full.h"
)
set(DTTR_PCDOGS_GENERATED_OUTPUTS
    ${DTTR_PCDOGS_GENERATED_PUBLIC_OUTPUTS}
    ${DTTR_PCDOGS_GENERATED_PRIVATE_OUTPUTS}
)
set(DTTR_SDK_BUNDLE_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/scripts/join_headers.py")
set(DTTR_SDK_BUNDLE_HEADER "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_sdk.h"
    CACHE INTERNAL "Generated DttR SDK bundle header"
)
set_property(GLOBAL PROPERTY DTTR_SDK_BUNDLE_HEADER "${DTTR_SDK_BUNDLE_HEADER}")

find_package(Python3 COMPONENTS Interpreter QUIET)
set(DTTR_PCDOGS_SCRIPT_RUNNER)
set(DTTR_PCDOGS_GENERATOR_COMMAND)
set(DTTR_PCDOGS_GENERATOR_AVAILABLE OFF)
if(Python3_Interpreter_FOUND)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import mako"
        RESULT_VARIABLE DTTR_PCDOGS_MAKO_STATUS
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(DTTR_PCDOGS_MAKO_STATUS EQUAL 0)
        set(DTTR_PCDOGS_SCRIPT_RUNNER
            "${CMAKE_COMMAND}" -E env
                PYTHONDONTWRITEBYTECODE=1
                "${Python3_EXECUTABLE}"
        )
    endif()
endif()

if(NOT DTTR_PCDOGS_SCRIPT_RUNNER)
    find_program(DTTR_NIX_EXECUTABLE nix)
    if(DTTR_NIX_EXECUTABLE)
        set(DTTR_PCDOGS_SCRIPT_RUNNER
            "${DTTR_NIX_EXECUTABLE}" develop "${CMAKE_SOURCE_DIR}" --command
                "${CMAKE_COMMAND}" -E env
                    PYTHONDONTWRITEBYTECODE=1
        )
    endif()
endif()

if(DTTR_PCDOGS_SCRIPT_RUNNER)
    set(DTTR_PCDOGS_GENERATOR_AVAILABLE ON)
    set(DTTR_PCDOGS_GENERATOR_COMMAND
        ${DTTR_PCDOGS_SCRIPT_RUNNER}
        "${DTTR_PCDOGS_GENERATOR}"
        --include-dir "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
        --src-dir "${DTTR_SDK_GENERATED_SRC_DIR}"
    )

    add_custom_command(
        OUTPUT ${DTTR_PCDOGS_GENERATED_OUTPUTS}
        COMMAND ${DTTR_PCDOGS_GENERATOR_COMMAND}
        DEPENDS ${DTTR_PCDOGS_GENERATOR_INPUTS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Regenerating PCDOGS SDK headers and implementation stubs"
        VERBATIM
    )
    add_custom_target(dttr_pcdogs_generated_headers
        DEPENDS ${DTTR_PCDOGS_GENERATED_OUTPUTS}
    )
else()
    message(FATAL_ERROR
        "PCDOGS SDK generator unavailable; install Python with Mako or nix so "
        "build-tree generated SDK outputs can be produced"
    )
endif()

add_custom_command(
    OUTPUT "${DTTR_SDK_BUNDLE_HEADER}"
    COMMAND ${DTTR_PCDOGS_SCRIPT_RUNNER}
            "${DTTR_SDK_BUNDLE_GENERATOR}"
            --include-dir "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
            --include-dir "${CMAKE_CURRENT_SOURCE_DIR}/include"
            --output "${DTTR_SDK_BUNDLE_HEADER}"
    DEPENDS
        dttr_pcdogs_generated_headers
        "${DTTR_SDK_BUNDLE_GENERATOR}"
        ${DTTR_SDK_STABLE_HEADERS}
        "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_pcdogs_unstable.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_util_unstable.h"
    COMMENT "Generating DttR SDK bundle header"
    VERBATIM
)
add_custom_target(dttr_sdk_bundle_header
    DEPENDS "${DTTR_SDK_BUNDLE_HEADER}"
)

add_library(dttr_sdk_runtime STATIC
    src/core.c
    src/hook_registry.c
)
set_source_files_properties(
    "${DTTR_SDK_GENERATED_SRC_DIR}/pcdogs.c"
    ${DTTR_PCDOGS_GENERATED_PRIVATE_OUTPUTS}
    PROPERTIES GENERATED TRUE
)
target_include_directories(dttr_sdk_runtime PUBLIC
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
add_dependencies(dttr_sdk_runtime dttr_pcdogs_generated_headers)
target_link_libraries(dttr_sdk_runtime
    PUBLIC
        common
    PRIVATE
        psapi
        Zydis
        klib
        xxhash
)

add_library(dttr_pcdogs_signatures STATIC
    "${DTTR_SDK_GENERATED_SRC_DIR}/pcdogs.c"
    src/pcdogs_symbols.c
    src/util_pkg_walk.c
)
target_include_directories(dttr_pcdogs_signatures PUBLIC
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
add_dependencies(dttr_pcdogs_signatures dttr_pcdogs_generated_headers)
target_link_libraries(dttr_pcdogs_signatures PUBLIC
    dttr_sdk_runtime
)

if(BUILD_TESTING)
    include("${CMAKE_CURRENT_LIST_DIR}/tests/tests.cmake")
endif()
