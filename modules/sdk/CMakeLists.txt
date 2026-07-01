# Locate the code generator scripts and the input files that should trigger regeneration.
set(DTTR_PCDOGS_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_headers.py")
set(DTTR_PCDOGS_METADATA_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_symbol_metadata.py")
set(DTTR_PCDOGS_GENERATOR_INPUTS
    "${CMAKE_SOURCE_DIR}/.clang-format"
    "${DTTR_PCDOGS_GENERATOR}"
    "${DTTR_PCDOGS_METADATA_GENERATOR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/codegen.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/blueprint.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/symbol_manifest.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/header_template.h.mako"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_versions.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/dttr_pcdogs.py"
)

set(DTTR_SDK_GENERATED_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/src")
set(DTTR_SDK_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include"
    CACHE INTERNAL "Generated DttR SDK include directory"
)
set(DTTR_SDK_GENERATED_METADATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/metadata"
    CACHE INTERNAL "Generated DttR SDK metadata directory"
)
file(MAKE_DIRECTORY "${DTTR_SDK_GENERATED_INCLUDE_DIR}")
file(MAKE_DIRECTORY "${DTTR_SDK_GENERATED_METADATA_DIR}")

set(DTTR_PCDOGS_GENERATED_PUBLIC_OUTPUTS
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_pcdogs.h"
)
set(DTTR_SDK_SYMBOL_MANIFEST "${DTTR_SDK_GENERATED_METADATA_DIR}/pcdogs.symbols.v1.json"
    CACHE INTERNAL "Generated DttR SDK symbol manifest"
)
set(DTTR_SDK_SYMBOL_SCHEMA "${DTTR_SDK_GENERATED_METADATA_DIR}/pcdogs.symbols.schema.json"
    CACHE INTERNAL "Generated DttR SDK symbol manifest schema"
)
set(DTTR_PCDOGS_GENERATED_METADATA_OUTPUTS
    "${DTTR_SDK_SYMBOL_MANIFEST}"
    "${DTTR_SDK_SYMBOL_SCHEMA}"
)

set(DTTR_SDK_STABLE_HEADERS
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_versions.h"
    "${CMAKE_SOURCE_DIR}/modules/common/include/dttr_result.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_runtime.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_core.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_mods.h"
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_pcdogs.h"
)
set(DTTR_SDK_UNSTABLE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_util_actor.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_util_pkg.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_util_mem.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_util_collision.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_util_worldview.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_util_gamedraw.h"
)

# Generate the versions header and read the SDK ABI version out of its template.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_versions.h.in"
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_versions.h"
    @ONLY
)
file(STRINGS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dttr_versions.h.in"
    DTTR_SDK_ABI_VERSION_DEFINE
    REGEX "^#define DTTR_SDK_ABI_VERSION "
)
string(REGEX REPLACE
    "^#define DTTR_SDK_ABI_VERSION ([0-9]+)u?.*$"
    "\\1"
    DTTR_SDK_ABI_VERSION
    "${DTTR_SDK_ABI_VERSION_DEFINE}"
)

set(DTTR_PCDOGS_GENERATED_PRIVATE_OUTPUTS
    "${DTTR_SDK_GENERATED_SRC_DIR}/pcdogs.c"
    "${DTTR_SDK_GENERATED_SRC_DIR}/generated/dttr_pcdogs_full.h"
)
set(DTTR_PCDOGS_GENERATED_OUTPUTS
    ${DTTR_PCDOGS_GENERATED_PUBLIC_OUTPUTS}
    ${DTTR_PCDOGS_GENERATED_PRIVATE_OUTPUTS}
    ${DTTR_PCDOGS_GENERATED_METADATA_OUTPUTS}
)

set(DTTR_SDK_BUNDLE_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/scripts/join_headers.py")
set(DTTR_SDK_BUNDLE_HEADER "${DTTR_SDK_GENERATED_INCLUDE_DIR}/dttr_sdk.h"
    CACHE INTERNAL "Generated DttR SDK bundle header"
)
set_property(GLOBAL PROPERTY DTTR_SDK_BUNDLE_HEADER "${DTTR_SDK_BUNDLE_HEADER}")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c
            "import mako, pefile, pydantic; assert int(pydantic.__version__.split('.')[0]) >= 2"
    RESULT_VARIABLE DTTR_PCDOGS_PYTHON_MODULE_STATUS
    ERROR_VARIABLE DTTR_PCDOGS_PYTHON_MODULE_ERROR
    OUTPUT_QUIET
)
if(NOT DTTR_PCDOGS_PYTHON_MODULE_STATUS EQUAL 0)
    message(FATAL_ERROR
        "PCDOGS SDK scripts require Python modules from requirements.txt "
        "(mako, pefile, pydantic>=2). Install them for ${Python3_EXECUTABLE} or use nix develop. "
        "Import error: ${DTTR_PCDOGS_PYTHON_MODULE_ERROR}"
    )
endif()

set(DTTR_PCDOGS_SCRIPT_RUNNER
    "${CMAKE_COMMAND}" -E env
        PYTHONDONTWRITEBYTECODE=1
        "${Python3_EXECUTABLE}"
)
set(DTTR_PCDOGS_GENERATOR_COMMAND
    ${DTTR_PCDOGS_SCRIPT_RUNNER}
    "${DTTR_PCDOGS_GENERATOR}"
    --include-dir "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    --src-dir "${DTTR_SDK_GENERATED_SRC_DIR}"
)
set(DTTR_PCDOGS_METADATA_GENERATOR_COMMAND
    ${DTTR_PCDOGS_SCRIPT_RUNNER}
    "${DTTR_PCDOGS_METADATA_GENERATOR}"
    --metadata-dir "${DTTR_SDK_GENERATED_METADATA_DIR}"
    --sdk-version "${DTTR_VERSION}"
    --sdk-abi-version "${DTTR_SDK_ABI_VERSION}"
    "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/dttr_pcdogs.py"
)

add_custom_command(
    OUTPUT ${DTTR_PCDOGS_GENERATED_OUTPUTS}
    COMMAND ${DTTR_PCDOGS_GENERATOR_COMMAND}
    COMMAND ${DTTR_PCDOGS_METADATA_GENERATOR_COMMAND}
    DEPENDS ${DTTR_PCDOGS_GENERATOR_INPUTS}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Regenerating PCDOGS SDK headers, implementation stubs, and metadata"
    VERBATIM
)
add_custom_target(dttr_pcdogs_generated_headers
    DEPENDS ${DTTR_PCDOGS_GENERATED_OUTPUTS}
)

set(_DTTR_SDK_UNSTABLE_HEADER_ARGS "")
foreach(_hdr IN LISTS DTTR_SDK_UNSTABLE_HEADERS)
    cmake_path(GET _hdr FILENAME _hdr_name)
    list(APPEND _DTTR_SDK_UNSTABLE_HEADER_ARGS --unstable-header "${_hdr_name}")
endforeach()

add_custom_command(
    OUTPUT "${DTTR_SDK_BUNDLE_HEADER}"
    COMMAND ${DTTR_PCDOGS_SCRIPT_RUNNER}
            "${DTTR_SDK_BUNDLE_GENERATOR}"
            --include-dir "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
            --include-dir "${CMAKE_SOURCE_DIR}/modules/common/include"
            --include-dir "${CMAKE_CURRENT_SOURCE_DIR}/include"
            ${_DTTR_SDK_UNSTABLE_HEADER_ARGS}
            --output "${DTTR_SDK_BUNDLE_HEADER}"
    DEPENDS
        dttr_pcdogs_generated_headers
        "${DTTR_SDK_BUNDLE_GENERATOR}"
        ${DTTR_SDK_STABLE_HEADERS}
        ${DTTR_SDK_UNSTABLE_HEADERS}
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
