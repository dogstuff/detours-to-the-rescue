dttr_add_test_group(dttr_common_tests)
dttr_check_cmocka_tests(common dttr_has_cmocka QUIET)
if(NOT dttr_has_cmocka)
    return()
endif()

set(DTTR_COMMON_TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")

dttr_add_cmocka_test_suite(dttr_common_core_tests
    GROUP
        dttr_common_tests
    SOURCES
        "${DTTR_COMMON_TEST_SOURCE_DIR}/src/core.c"
    LINK_LIBRARIES
        common
    LABELS
        common
    RUNTIME_FILES
        ${DTTR_SDL3_RUNTIME_DLL}
)

dttr_add_cmocka_test_suite(dttr_common_config_tests
    GROUP
        dttr_common_tests
    SOURCES
        "${DTTR_COMMON_TEST_SOURCE_DIR}/src/config.c"
    LINK_LIBRARIES
        common
    LABELS
        common
    RUNTIME_FILES
        ${DTTR_SDL3_RUNTIME_DLL}
)

add_library(dttr_test_support STATIC
    "${DTTR_COMMON_TEST_SOURCE_DIR}/src/binary.c"
)
target_include_directories(dttr_test_support PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${DTTR_COMMON_TEST_SOURCE_DIR}/include"
)
target_link_libraries(dttr_test_support PUBLIC
    ${DTTR_CMOCKA_TARGET}
    common
    physfs-static
    sds
    xxhash
    Zydis
)
