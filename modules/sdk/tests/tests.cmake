set(DTTR_SDK_TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DTTR_SDK_TEST_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests")
set(DTTR_SDK_TEST_INCLUDE_DIRS
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    "${DTTR_SDK_GENERATED_SRC_DIR}/generated"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/include"
)

dttr_add_object_compile_check(dttr_sdk_compile_checks
    SOURCES
        "${DTTR_SDK_TEST_SOURCE_DIR}/pcdogs_unstable_impl.c"
        "${DTTR_SDK_TEST_SOURCE_DIR}/bundle.c"
        "${DTTR_SDK_TEST_SOURCE_DIR}/bundle_unstable.c"
    INCLUDE_DIRS
        ${DTTR_SDK_TEST_INCLUDE_DIRS}
)
add_dependencies(dttr_sdk_compile_checks
    dttr_pcdogs_generated_headers
    dttr_sdk_bundle_header
)

add_executable(dttr_runtime_cpp_link_check
    "${DTTR_SDK_TEST_SOURCE_DIR}/runtime_cpp_link_check.cc"
)
target_link_libraries(dttr_runtime_cpp_link_check PRIVATE
    dttr_sdk_runtime
)
add_dependencies(dttr_runtime_cpp_link_check
    dttr_sdk_bundle_header
)
add_test(
    NAME dttr_sdk_runtime_cpp_link_check
    COMMAND dttr_runtime_cpp_link_check
)
set_tests_properties(dttr_sdk_runtime_cpp_link_check PROPERTIES
    LABELS "sdk"
)

add_custom_target(dttr_sdk_tests
    DEPENDS
        dttr_sdk_compile_checks
        dttr_runtime_cpp_link_check
)
add_dependencies(dttr_tests dttr_sdk_tests)

if(DTTR_PCDOGS_GENERATOR_AVAILABLE)
    add_test(
        NAME dttr_pcdogs_generated_headers
        COMMAND ${DTTR_PCDOGS_GENERATOR_COMMAND} --check
    )
    set_tests_properties(dttr_pcdogs_generated_headers PROPERTIES
        LABELS "sdk;pcdogs;generated"
    )

    add_test(
        NAME dttr_sdk_bundle_header
        COMMAND ${DTTR_PCDOGS_SCRIPT_RUNNER}
            "${DTTR_SDK_BUNDLE_GENERATOR}"
            --include-dir "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
            --include-dir "${CMAKE_CURRENT_SOURCE_DIR}/include"
            --output "${DTTR_SDK_BUNDLE_HEADER}"
            --check
    )
    set_tests_properties(dttr_sdk_bundle_header PROPERTIES
        LABELS "sdk;generated"
    )
else()
    message(WARNING
        "PCDOGS SDK generator unavailable; skipping generated SDK freshness tests"
    )
endif()

if(NOT DTTR_CMOCKA_FOUND)
    if(DTTR_REQUIRE_TEST_DEPS)
        message(FATAL_ERROR "cmocka is required when DTTR_REQUIRE_TEST_DEPS=ON")
    endif()

    message(WARNING "cmocka was not found; skipping DttR cmocka tests")
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
    SOURCES
        "${DTTR_SDK_TEST_SOURCE_DIR}/core.c"
    INCLUDE_DIRS
        ${DTTR_SDK_TEST_INCLUDE_DIRS}
    LINK_LIBRARIES
        dttr_pcdogs_signatures
        dttr_sdk_runtime
    LABELS
        sdk
)

dttr_add_cmocka_test_suite(dttr_hook_registry_tests
    SOURCES
        "${DTTR_SDK_TEST_SOURCE_DIR}/hook_registry.c"
    INCLUDE_DIRS
        ${DTTR_SDK_TEST_INCLUDE_DIRS}
    LINK_LIBRARIES
        dttr_sdk_runtime
    LABELS
        sdk
)

add_dependencies(dttr_sdk_tests
    dttr_core_sdk_tests
    dttr_hook_registry_tests
)

if(DTTR_PCDOGS_GENERATOR_AVAILABLE)
    set(DTTR_PCDOGS_BLUEPRINT_TEST_ROWS "${DTTR_SDK_TEST_BINARY_DIR}/pcdogs_blueprint_test_rows.h")
    set(DTTR_PCDOGS_TEST_GENERATOR_ARGS
        --rows-output "${DTTR_PCDOGS_BLUEPRINT_TEST_ROWS}"
        "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/dttr_pcdogs.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/dttr_pcdogs_unstable.py"
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
            "${CMAKE_CURRENT_SOURCE_DIR}/blueprints/dttr_pcdogs_unstable.py"
        COMMENT "Generating PCDOGS blueprint test rows"
        VERBATIM
    )
    add_custom_target(dttr_pcdogs_blueprint_test_rows
        DEPENDS "${DTTR_PCDOGS_BLUEPRINT_TEST_ROWS}"
    )

    dttr_add_cmocka_test_suite(dttr_pcdogs_sig_tests
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
        TIMEOUT 300
    )

    add_dependencies(dttr_pcdogs_sig_tests dttr_pcdogs_blueprint_test_rows)
    add_dependencies(dttr_sdk_tests dttr_pcdogs_sig_tests)
else()
    message(WARNING
        "PCDOGS SDK generator unavailable; skipping generated blueprint signature tests"
    )
endif()
