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
    SOURCES
        "${DTTR_SIDECAR_TEST_SOURCE_DIR}/pcdogs.c"
    INCLUDE_DIRS
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

dttr_add_cmocka_test_suite(dttr_sidecar_directdraw_validation_tests
    SOURCES
        "${DTTR_SIDECAR_TEST_SOURCE_DIR}/directdraw_validation.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/graphics/com_directdraw7.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/graphics/com_directdrawsurface7.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/graphics/com_direct3dtexture2.c"
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/include"
        "${CMAKE_SOURCE_DIR}/modules/sdk/include"
    LINK_LIBRARIES
        PkgConfig::SDL3
        common
        klib
        xxhash
        dxguid
    LABELS
        sidecar
        directdraw
)

dttr_add_cmocka_test_suite(dttr_sidecar_mesh_seam_fill_tests
    SOURCES
        "${DTTR_SIDECAR_TEST_SOURCE_DIR}/mesh_seam_fill.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/graphics/util.c"
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/include"
        "${CMAKE_SOURCE_DIR}/modules/sdk/include"
    LINK_LIBRARIES
        PkgConfig::SDL3
        common
        klib
    LABELS
        sidecar
        graphics
)

dttr_copy_runtime_files(
    dttr_sidecar_directdraw_validation_tests
    "$<TARGET_FILE_DIR:dttr_sidecar_directdraw_validation_tests>"
    FILES
        ${DTTR_SDL3_RUNTIME_DLL}
)

dttr_copy_runtime_files(
    dttr_sidecar_mesh_seam_fill_tests
    "$<TARGET_FILE_DIR:dttr_sidecar_mesh_seam_fill_tests>"
    FILES
        ${DTTR_SDL3_RUNTIME_DLL}
)

add_dependencies(dttr_sidecar_tests
    dttr_sidecar_pcdogs_tests
    dttr_sidecar_directdraw_validation_tests
    dttr_sidecar_mesh_seam_fill_tests
)
