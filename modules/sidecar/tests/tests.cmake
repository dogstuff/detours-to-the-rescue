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

dttr_add_cmocka_test_suite(dttr_sidecar_key_state_tests
    GROUP
        dttr_sidecar_tests
    SOURCES
        "${DTTR_SIDECAR_TEST_SOURCE_DIR}/src/key_state.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/inputs/controls_menu.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/inputs/hook_getasynckeystate.c"
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src"
        "${CMAKE_SOURCE_DIR}/modules/sdk/include"
        "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    LINK_LIBRARIES
        dttr_sdk_runtime
        PkgConfig::SDL3
    LABELS
        sidecar
        inputs
    RUNTIME_FILES
        ${DTTR_SDL3_RUNTIME_DLL}
)
target_compile_definitions(dttr_sidecar_key_state_tests PRIVATE DTTR_SDK_ENABLE_UNSTABLE)
