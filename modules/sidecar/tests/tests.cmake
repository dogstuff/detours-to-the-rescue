set(DTTR_SIDECAR_TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")

dttr_add_test_group(dttr_sidecar_tests)
dttr_check_cmocka_tests(sidecar dttr_has_cmocka)
if(NOT dttr_has_cmocka)
    return()
endif()

dttr_add_cmocka_test_suite(dttr_sidecar_pcdogs_tests
    GROUP
        dttr_sidecar_tests
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
