set(DTTR_SDK_TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DTTR_SDK_TEST_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests")
set(DTTR_SDK_TEST_INCLUDE_DIRS
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    "${DTTR_SDK_GENERATED_SRC_DIR}/generated"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/include"
)

dttr_add_test_group(dttr_sdk_tests)

add_test(
    NAME dttr_pcdogs_generated_headers
    COMMAND ${DTTR_PCDOGS_GENERATOR_COMMAND} --check
)
set_tests_properties(dttr_pcdogs_generated_headers PROPERTIES
    LABELS "sdk;pcdogs;generated"
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
