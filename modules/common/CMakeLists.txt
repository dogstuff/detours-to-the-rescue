# Builds shared runtime helpers used by loader, config UI, and tests.
dttr_find_sdl3()

add_library(common STATIC
    src/log.c
    src/errors.c
    src/crashdump.c
    src/imgui_dialog.c
    src/imgui_error_ui.c
    src/path.c
    src/sdl.c
    src/iso/iso.c
    src/config/defaults.c
    src/config/io.c
    src/config/parse.c
    src/config/schema.c
)
target_include_directories(common PUBLIC
    include
    ${SDL3_INCLUDE_DIRS}
)
target_compile_options(common PUBLIC
    ${SDL3_CFLAGS_OTHER}
)
target_link_libraries(common
    PUBLIC
        cimgui_static
        log
        PkgConfig::SDL3
        sds
    PRIVATE
        dbghelp
        glad
        klib
        physfs-static
        yyjson
)
target_compile_definitions(common PRIVATE
    DTTR_DEFAULT_LOG_LEVEL=$<IF:$<CONFIG:debug>,LOG_DEBUG,LOG_INFO>
    DTTR_DEFAULT_MINIDUMP_TYPE=$<IF:$<CONFIG:debug>,DTTR_MINIDUMP_DETAILED,DTTR_MINIDUMP_NORMAL>
    DTTR_VERSION="${DTTR_VERSION}"
)

# Builds binary-fixture support used by module test executables.
if(BUILD_TESTING)
    add_custom_target(dttr_common_tests)
    add_dependencies(dttr_tests dttr_common_tests)

    if(NOT DTTR_CMOCKA_FOUND AND DTTR_REQUIRE_TEST_DEPS)
        message(FATAL_ERROR "cmocka is required when DTTR_REQUIRE_TEST_DEPS=ON")
    endif()

    if(DTTR_CMOCKA_FOUND)
        dttr_add_cmocka_test_suite(dttr_common_core_tests
            SOURCES
                tests/src/core.c
            LINK_LIBRARIES
                common
            LABELS
                common
        )
        dttr_copy_runtime_files(
            dttr_common_core_tests
            "$<TARGET_FILE_DIR:dttr_common_core_tests>"
            FILES
                ${DTTR_SDL3_RUNTIME_DLL}
        )
        add_dependencies(dttr_common_tests dttr_common_core_tests)

        add_library(dttr_test_support STATIC
            tests/src/binary.c
        )
        target_include_directories(dttr_test_support PUBLIC
            include
            tests/include
        )
        target_link_libraries(dttr_test_support PUBLIC
            ${DTTR_CMOCKA_TARGET}
            physfs-static
            sds
            xxhash
            Zydis
        )
    endif()
endif()
