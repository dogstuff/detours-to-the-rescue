dttr_add_test_group(dttr_config_gui_tests)
dttr_check_cmocka_tests(config_gui dttr_has_cmocka QUIET)
if(NOT dttr_has_cmocka)
    return()
endif()

set(DTTR_CONFIG_GUI_TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")

dttr_add_cmocka_test_suite(dttr_config_gui_mod_path_tests
    GROUP
        dttr_config_gui_tests
    SOURCES
        "${DTTR_CONFIG_GUI_TEST_SOURCE_DIR}/src/mod_load_path.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gui_mod_load_path.c"
    INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    LINK_LIBRARIES
        common
        dttr_mod_host
    LABELS
        config_gui
    RUNTIME_FILES
        ${DTTR_SDL3_RUNTIME_DLL}
)
