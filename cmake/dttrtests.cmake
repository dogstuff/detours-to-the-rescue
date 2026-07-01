# Shared CTest and cmocka helpers for DttR's first-party test suites.

# Locates cmocka for Windows test executables and records the DLL directory for Wine runs.
function(dttr_find_cmocka)
    find_package(cmocka CONFIG QUIET PATHS "${CMAKE_SOURCE_DIR}/.toolchain/cmocka")

    set(dttr_cmocka_target "")
    set(dttr_cmocka_runtime_dir "")

    if(TARGET cmocka::cmocka)
        set(dttr_cmocka_target cmocka::cmocka)
        set(dttr_cmocka_runtime_dir "$<TARGET_FILE_DIR:cmocka::cmocka>")
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(CMOCKA IMPORTED_TARGET cmocka)
        endif()

        if(CMOCKA_FOUND)
            set(dttr_cmocka_target PkgConfig::CMOCKA)
            find_file(dttr_cmocka_runtime_dll
                NAMES cmocka.dll
                HINTS
                    "${CMOCKA_PREFIX}/bin"
                    "${CMOCKA_BINDIR}"
                    "${CMOCKA_LIBDIR}/../bin"
                NO_DEFAULT_PATH
            )
            if(dttr_cmocka_runtime_dll)
                get_filename_component(
                    dttr_cmocka_runtime_dir
                    "${dttr_cmocka_runtime_dll}"
                    DIRECTORY
                )
            endif()
        endif()
    endif()

    if(NOT dttr_cmocka_target)
        set(DTTR_CMOCKA_FOUND FALSE PARENT_SCOPE)
        set(DTTR_CMOCKA_TARGET "" PARENT_SCOPE)
        set(DTTR_CMOCKA_RUNTIME_DIR "" PARENT_SCOPE)
        return()
    endif()

    set(DTTR_CMOCKA_FOUND TRUE PARENT_SCOPE)
    set(DTTR_CMOCKA_TARGET ${dttr_cmocka_target} PARENT_SCOPE)
    set(DTTR_CMOCKA_RUNTIME_DIR "${dttr_cmocka_runtime_dir}" PARENT_SCOPE)
endfunction()

function(dttr_cmocka_test_environment out_var)
    set(dttr_environment "PCDOGS_FIXTURES_DIR=${PCDOGS_FIXTURES_DIR}")
    if(DTTR_CMOCKA_RUNTIME_DIR)
        list(APPEND dttr_environment "WINEPATH=${DTTR_CMOCKA_RUNTIME_DIR}")
    endif()

    if(PCDOGS_FIXTURES_REQUIRED)
        list(APPEND dttr_environment "PCDOGS_FIXTURES_REQUIRED=1")
    endif()

    set(${out_var} ${dttr_environment} PARENT_SCOPE)
endfunction()

function(dttr_add_test_group target)
    if(NOT TARGET "${target}")
        add_custom_target("${target}")
    endif()

    add_dependencies(dttr_tests "${target}")
endfunction()

function(dttr_check_cmocka_tests label out_var)
    cmake_parse_arguments(DTTR_CHECK "QUIET" "MESSAGE" "" ${ARGN})

    if(NOT DTTR_CMOCKA_FOUND)
        if(TEST_DEPS_REQUIRED)
            message(FATAL_ERROR "cmocka is required when TEST_DEPS_REQUIRED=ON")
        endif()

        if(NOT DTTR_CHECK_QUIET)
            if(DTTR_CHECK_MESSAGE)
                message(WARNING "${DTTR_CHECK_MESSAGE}")
            else()
                message(WARNING "cmocka was not found; skipping DttR ${label} cmocka tests")
            endif()
        endif()

        set(${out_var} FALSE PARENT_SCOPE)
        return()
    endif()

    set(${out_var} TRUE PARENT_SCOPE)
endfunction()

# Registers a cmocka target with the repo's standard CTest settings.
function(dttr_add_cmocka_test_suite target)
    cmake_parse_arguments(
        DTTR_SUITE
        ""
        "GROUP;TIMEOUT"
        "SOURCES;INCLUDE_DIRS;LINK_LIBRARIES;LABELS;DEPENDS;RUNTIME_FILES"
        ${ARGN}
    )

    add_executable("${target}"
        ${DTTR_SUITE_SOURCES}
    )
    target_include_directories("${target}" PRIVATE
        "${CMAKE_SOURCE_DIR}/modules/common/tests/include"
        ${DTTR_SUITE_INCLUDE_DIRS}
    )
    target_link_libraries("${target}" PRIVATE
        ${DTTR_CMOCKA_TARGET}
        ${DTTR_SUITE_LINK_LIBRARIES}
    )

    if(NOT DTTR_SUITE_TIMEOUT)
        set(DTTR_SUITE_TIMEOUT 120)
    endif()

    dttr_cmocka_test_environment(dttr_suite_environment)
    add_test(NAME "${target}" COMMAND "${target}")
    set_tests_properties("${target}" PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
        TIMEOUT "${DTTR_SUITE_TIMEOUT}"
    )
    if(dttr_suite_environment)
        set_property(TEST "${target}" PROPERTY ENVIRONMENT ${dttr_suite_environment})
    endif()

    if(DTTR_SUITE_LABELS)
        set_property(TEST "${target}" PROPERTY LABELS ${DTTR_SUITE_LABELS})
    endif()

    if(DTTR_SUITE_DEPENDS)
        add_dependencies("${target}" ${DTTR_SUITE_DEPENDS})
    endif()

    if(DTTR_SUITE_RUNTIME_FILES)
        dttr_copy_runtime_files("${target}" "$<TARGET_FILE_DIR:${target}>"
            FILES
                ${DTTR_SUITE_RUNTIME_FILES}
        )
    endif()

    if(DTTR_SUITE_GROUP)
        add_dependencies("${DTTR_SUITE_GROUP}" "${target}")
    endif()
endfunction()
