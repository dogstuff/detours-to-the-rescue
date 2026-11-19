set(DTTR_SDK_TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DTTR_SDK_TEST_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests")
set(DTTR_SDK_TEST_INCLUDE_DIRS
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    "${DTTR_SDK_GENERATED_SRC_DIR}/generated"
    "${CMAKE_SOURCE_DIR}/modules/common/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/include"
)

dttr_add_test_group(dttr_sdk_tests)

add_dependencies(dttr_sdk_tests dttr_sdk_bundle_header)

function(dttr_test_sdk_header name header)
    add_test(NAME "dttr_sdk_header_${name}"
        COMMAND "${CMAKE_C_COMPILER}" -std=gnu${CMAKE_C_STANDARD} -x c -fsyntax-only
            ${ARGN} "${header}"
    )
    set_tests_properties("dttr_sdk_header_${name}" PROPERTIES LABELS "sdk;headers")
endfunction()

foreach(header IN LISTS DTTR_SDK_STABLE_HEADERS DTTR_SDK_UNSTABLE_HEADERS)
    get_filename_component(name "${header}" NAME_WE)
    set(flags "")
    if(header IN_LIST DTTR_SDK_UNSTABLE_HEADERS)
        list(APPEND flags -DDTTR_SDK_ENABLE_UNSTABLE)
    endif()
    if(name STREQUAL "dttr_util_gamedraw")
        list(APPEND flags -include d3d.h)
    endif()
    dttr_test_sdk_header("${name}" "${header}" ${flags}
        "-I${DTTR_SDK_GENERATED_INCLUDE_DIR}"
        "-I${CMAKE_SOURCE_DIR}/modules/common/include"
        "-I${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
endforeach()

# Consumers can use the bundle without any other SDK include directories.
dttr_test_sdk_header(bundle "${DTTR_SDK_BUNDLE_HEADER}")
dttr_test_sdk_header(bundle_unstable "${DTTR_SDK_BUNDLE_HEADER}"
    -DDTTR_SDK_ENABLE_UNSTABLE -include d3d.h
)

add_test(
    NAME dttr_pcdogs_generated_headers
    COMMAND ${DTTR_PCDOGS_GENERATOR_COMMAND} --check
)
add_test(
    NAME dttr_pcdogs_symbol_metadata
    COMMAND ${DTTR_PCDOGS_METADATA_GENERATOR_COMMAND} --check
)
add_test(
    NAME dttr_pcdogs_symbol_manifest
    COMMAND ${DTTR_PCDOGS_SCRIPT_RUNNER}
        "${DTTR_SDK_TEST_SOURCE_DIR}/check_symbol_manifest.py"
        --manifest "${DTTR_SDK_SYMBOL_MANIFEST}"
        --schema "${DTTR_SDK_SYMBOL_SCHEMA}"
        --header "${DTTR_SDK_BUNDLE_HEADER}"
)
set_tests_properties(
    dttr_pcdogs_generated_headers
    dttr_pcdogs_symbol_metadata
    dttr_pcdogs_symbol_manifest
    PROPERTIES LABELS "sdk;pcdogs;generated"
)

dttr_check_cmocka_tests(sdk dttr_has_cmocka
    MESSAGE "cmocka was not found; skipping DttR cmocka tests"
)
if(NOT dttr_has_cmocka)
    return()
endif()

add_library(dttr_pcdogs_test_fixtures OBJECT
    "${DTTR_SDK_TEST_SOURCE_DIR}/pcdogs_fixtures.c"
)
target_include_directories(dttr_pcdogs_test_fixtures PUBLIC
    ${DTTR_SDK_TEST_INCLUDE_DIRS}
)
target_link_libraries(dttr_pcdogs_test_fixtures PUBLIC
    dttr_test_support
)
add_dependencies(dttr_pcdogs_test_fixtures
    dttr_pcdogs_generated_headers
)

dttr_add_cmocka_test_suite(dttr_core_sdk_tests
    GROUP
        dttr_sdk_tests
    SOURCES
        "${DTTR_SDK_TEST_SOURCE_DIR}/core.c"
    INCLUDE_DIRS
        ${DTTR_SDK_TEST_INCLUDE_DIRS}
    LINK_LIBRARIES
        dttr_sdk_runtime
        dttr_pcdogs_signatures
    LABELS
        sdk
)

dttr_add_cmocka_test_suite(dttr_hook_registry_tests
    GROUP
        dttr_sdk_tests
    SOURCES
        "${DTTR_SDK_TEST_SOURCE_DIR}/hook_registry.c"
    INCLUDE_DIRS
        ${DTTR_SDK_TEST_INCLUDE_DIRS}
    LINK_LIBRARIES
        dttr_sdk_runtime
    LABELS
        sdk
        hooks
)

set(DTTR_PCDOGS_BLUEPRINT_TEST_ROWS "${DTTR_SDK_TEST_BINARY_DIR}/pcdogs_blueprint_test_rows.h")
set(DTTR_PCDOGS_TEST_GENERATOR_ARGS
    --rows-output "${DTTR_PCDOGS_BLUEPRINT_TEST_ROWS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/dttr_pcdogs.py"
)
add_custom_command(
    OUTPUT "${DTTR_PCDOGS_BLUEPRINT_TEST_ROWS}"
    COMMAND ${DTTR_PCDOGS_SCRIPT_RUNNER}
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_tests.py"
        ${DTTR_PCDOGS_TEST_GENERATOR_ARGS}
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_tests.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/codegen.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/blueprint.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/dttr_pcdogs.py"
        COMMENT "Generating PCDOGS blueprint test rows"
    VERBATIM
)
add_custom_target(dttr_pcdogs_blueprint_test_rows
    DEPENDS "${DTTR_PCDOGS_BLUEPRINT_TEST_ROWS}"
)

dttr_add_cmocka_test_suite(dttr_pcdogs_sig_tests
    GROUP
        dttr_sdk_tests
    SOURCES
        "${DTTR_SDK_TEST_SOURCE_DIR}/pcdogs.c"
        "${DTTR_PCDOGS_BLUEPRINT_TEST_ROWS}"
    INCLUDE_DIRS
        "${DTTR_SDK_TEST_BINARY_DIR}"
        ${DTTR_SDK_TEST_INCLUDE_DIRS}
    LINK_LIBRARIES
        dttr_pcdogs_signatures
        dttr_pcdogs_test_fixtures
        dttr_test_support
    LABELS
        sdk
        pcdogs
        fixtures
    DEPENDS
        dttr_pcdogs_blueprint_test_rows
    TIMEOUT 300
)
