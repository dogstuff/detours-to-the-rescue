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

function(dttr_set_cmocka_test_properties target test_name labels environment timeout)
    set_tests_properties("${test_name}" PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
        TIMEOUT "${timeout}"
    )
    if(environment)
        set_property(TEST "${test_name}" PROPERTY ENVIRONMENT ${environment})
    endif()
    if(labels)
        set_property(TEST "${test_name}" PROPERTY LABELS ${labels})
    endif()
endfunction()

# Registers cmocka tests with CTest. Prefer one CTest entry per cmocka case.
function(dttr_add_cmocka_suite_test target labels cases timeout)
    set(dttr_test_environment "")
    set(dttr_winepath_dirs "")
    if(DTTR_CMOCKA_RUNTIME_DIR)
        list(APPEND dttr_winepath_dirs "${DTTR_CMOCKA_RUNTIME_DIR}")
    endif()
    if(dttr_winepath_dirs)
        list(JOIN dttr_winepath_dirs "\\;" dttr_winepath)
        list(APPEND dttr_test_environment "WINEPATH=${dttr_winepath}")
    endif()
    list(APPEND dttr_test_environment "DTTR_PCDOGS_FIXTURE_DIR=${DTTR_PCDOGS_FIXTURE_DIR}")
    if(DTTR_REQUIRE_PCDOGS_FIXTURES)
        list(APPEND dttr_test_environment "DTTR_REQUIRE_PCDOGS_FIXTURES=1")
    endif()

    if(cases)
        foreach(test_case IN LISTS cases)
            set(test_name "${target}.${test_case}")
            add_test(NAME "${test_name}" COMMAND ${target} "${test_case}")
            dttr_set_cmocka_test_properties(
                "${target}"
                "${test_name}"
                "${labels}"
                "${dttr_test_environment}"
                "${timeout}"
            )
        endforeach()
        return()
    endif()

    add_test(NAME "${target}" COMMAND ${target})
    dttr_set_cmocka_test_properties(
        "${target}"
        "${target}"
        "${labels}"
        "${dttr_test_environment}"
        "${timeout}"
    )
endfunction()

# Applies the shared test headers and cmocka library to one test executable.
function(dttr_configure_cmocka_test target)
    target_include_directories("${target}" PRIVATE
        "${CMAKE_SOURCE_DIR}/modules/common/tests/include"
    )
    target_link_libraries("${target}" PRIVATE
        ${DTTR_CMOCKA_TARGET}
    )
endfunction()

# Adds an object library used only to verify headers and macros compile.
function(dttr_add_object_compile_check target)
    cmake_parse_arguments(
        DTTR_CHECK
        ""
        ""
        "SOURCES;INCLUDE_DIRS;COMPILE_DEFINITIONS;LINK_LIBRARIES"
        ${ARGN}
    )

    add_library("${target}" OBJECT
        ${DTTR_CHECK_SOURCES}
    )
    if(DTTR_CHECK_INCLUDE_DIRS)
        target_include_directories("${target}" PRIVATE
            ${DTTR_CHECK_INCLUDE_DIRS}
        )
    endif()
    if(DTTR_CHECK_COMPILE_DEFINITIONS)
        target_compile_definitions("${target}" PRIVATE
            ${DTTR_CHECK_COMPILE_DEFINITIONS}
        )
    endif()
    if(DTTR_CHECK_LINK_LIBRARIES)
        target_link_libraries("${target}" PRIVATE
            ${DTTR_CHECK_LINK_LIBRARIES}
        )
    endif()
endfunction()

# Adds a cmocka executable, applies common test settings, and registers it with ctest.
function(dttr_add_cmocka_test_suite target)
    cmake_parse_arguments(
        DTTR_SUITE
        ""
        "TIMEOUT"
        "SOURCES;INCLUDE_DIRS;LINK_LIBRARIES;LABELS;CASES"
        ${ARGN}
    )

    add_executable("${target}"
        ${DTTR_SUITE_SOURCES}
    )
    if(DTTR_SUITE_INCLUDE_DIRS)
        target_include_directories("${target}" PRIVATE
            ${DTTR_SUITE_INCLUDE_DIRS}
        )
    endif()
    if(DTTR_SUITE_LINK_LIBRARIES)
        target_link_libraries("${target}" PRIVATE
            ${DTTR_SUITE_LINK_LIBRARIES}
        )
    endif()

    if(NOT DTTR_SUITE_TIMEOUT)
        set(DTTR_SUITE_TIMEOUT 120)
    endif()

    dttr_configure_cmocka_test("${target}")
    dttr_add_cmocka_suite_test(
        "${target}"
        "${DTTR_SUITE_LABELS}"
        "${DTTR_SUITE_CASES}"
        "${DTTR_SUITE_TIMEOUT}"
    )
endfunction()
