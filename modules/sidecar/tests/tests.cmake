set(DTTR_SIDECAR_TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")

add_custom_target(dttr_sidecar_tests)
add_dependencies(dttr_tests dttr_sidecar_tests)

if(NOT DTTR_CMOCKA_FOUND)
    if(DTTR_REQUIRE_TEST_DEPS)
        message(FATAL_ERROR "cmocka is required when DTTR_REQUIRE_TEST_DEPS=ON")
    endif()

    message(WARNING "cmocka was not found; skipping DttR sidecar cmocka tests")
    return()
endif()

dttr_add_cmocka_test_suite(dttr_sidecar_pcdogs_tests
    TIMEOUT
        300
    SOURCES
        "${DTTR_SIDECAR_TEST_SOURCE_DIR}/pcdogs.c"
    INCLUDE_DIRS
        "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/modules/sdk/include"
        "${CMAKE_SOURCE_DIR}/modules/sdk/tests/include"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/include"
    LINK_LIBRARIES
        dttr_pcdogs_test_fixtures
        dttr_test_support
    LABELS
        sidecar
        pcdogs
        fixtures
)

add_dependencies(dttr_sidecar_tests
    dttr_sidecar_pcdogs_tests
)
