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
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/inputs/byte_patches.c"
    INCLUDE_DIRS
        "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/modules/sdk/include"
        "${CMAKE_SOURCE_DIR}/modules/sdk/tests/include"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/include"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src"
    LINK_LIBRARIES
        dttr_pcdogs_test_fixtures
        dttr_sdk_runtime
        dttr_test_support
    LABELS
        sidecar
        pcdogs
        fixtures
)
target_compile_definitions(dttr_sidecar_pcdogs_tests PRIVATE DTTR_SDK_ENABLE_UNSTABLE)

dttr_add_cmocka_test_suite(dttr_sidecar_key_state_tests
    GROUP
        dttr_sidecar_tests
    SOURCES
        "${DTTR_SIDECAR_TEST_SOURCE_DIR}/src/key_state.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/inputs/controls_menu.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/inputs/hook_control_bindings.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/inputs/hook_getasynckeystate.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/inputs/hook_rumble.c"
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src"
        "${CMAKE_SOURCE_DIR}/modules/sdk/include"
        "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    LINK_LIBRARIES
        dttr_sdk_runtime
        dttr_pcdogs_signatures
        common
        PkgConfig::SDL3
    LABELS
        sidecar
        inputs
    RUNTIME_FILES
        ${DTTR_SDL3_RUNTIME_DLL}
)
target_compile_definitions(dttr_sidecar_key_state_tests PRIVATE DTTR_SDK_ENABLE_UNSTABLE)
target_link_options(dttr_sidecar_key_state_tests PRIVATE
    -Wl,--wrap=SDL_RumbleGamepad
    -Wl,--wrap=SDL_GetGamepadButton
)

dttr_add_cmocka_test_suite(dttr_sidecar_events_tests
    GROUP
        dttr_sidecar_tests
    SOURCES
        "${DTTR_SIDECAR_TEST_SOURCE_DIR}/src/events.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/events.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/graphics/resize.c"
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src"
        "${CMAKE_SOURCE_DIR}/modules/common/include"
        "${CMAKE_SOURCE_DIR}/modules/sdk/include"
        "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    LINK_LIBRARIES
        dttr_pcdogs_signatures
        klib
        common
        PkgConfig::SDL3
    LABELS
        sidecar
        events
        graphics
    RUNTIME_FILES
        ${DTTR_SDL3_RUNTIME_DLL}
)
target_compile_definitions(dttr_sidecar_events_tests PRIVATE DTTR_SDK_ENABLE_UNSTABLE)
target_link_options(dttr_sidecar_events_tests PRIVATE
    -Wl,--wrap=SDL_PollEvent
    -Wl,--wrap=SDL_GetWindowID
)

dttr_add_cmocka_test_suite(dttr_sidecar_resize_tests
    GROUP
        dttr_sidecar_tests
    SOURCES
        "${DTTR_SIDECAR_TEST_SOURCE_DIR}/src/resize.c"
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src/graphics/resize.c"
    INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/modules/sidecar/src"
        "${CMAKE_SOURCE_DIR}/modules/common/include"
    LINK_LIBRARIES
        common
    LABELS
        sidecar
        graphics
)
