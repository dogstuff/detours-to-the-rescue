dttr_add_test_group(dttr_mod_host_tests)
dttr_check_cmocka_tests(mod_host dttr_has_cmocka QUIET)
if(NOT dttr_has_cmocka)
    return()
endif()

set(DTTR_MOD_HOST_TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")

dttr_add_cmocka_test_suite(dttr_mod_host_input_binding_tests
    GROUP
        dttr_mod_host_tests
    SOURCES
        "${DTTR_MOD_HOST_TEST_SOURCE_DIR}/src/input_binding.c"
    LINK_LIBRARIES
        dttr_mod_host
    LABELS
        common
        mod_host
    DEPENDS
        dttr_pcdogs_generated_headers
    RUNTIME_FILES
        ${DTTR_SDL3_RUNTIME_DLL}
)
