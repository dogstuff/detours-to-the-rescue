dttr_find_sdl3()

add_library(dttr_mod_host STATIC
    src/input_binding.c
)
add_dependencies(dttr_mod_host dttr_pcdogs_generated_headers)
target_include_directories(dttr_mod_host PUBLIC
    include
    "${CMAKE_SOURCE_DIR}/modules/sdk/include"
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
)
target_compile_options(dttr_mod_host PUBLIC
    ${SDL3_CFLAGS_OTHER}
)
target_link_libraries(dttr_mod_host
    PUBLIC
        common
        PkgConfig::SDL3
)

if(BUILD_TESTING)
    include("${CMAKE_CURRENT_LIST_DIR}/tests/tests.cmake")
endif()
