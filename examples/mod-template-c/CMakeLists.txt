cmake_minimum_required(VERSION 3.20)

set(CMAKE_CONFIGURATION_TYPES "debug;release" CACHE STRING "Available build configurations" FORCE)

project(dttr_minimal_mod VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(DTTRSDK CONFIG QUIET)
if(NOT DTTRSDK_FOUND)
    message(FATAL_ERROR
        "Could not find the DttR SDK.\n"
        "Run ./scripts/fetch-dttr.sh on Linux/macOS/Nix, "
        "run ./scripts/fetch-dttr.ps1 on Windows, "
        "or configure with -DDTTRSDK_DIR=/path/to/dttr/sdk."
    )
endif()

add_library(dttr_minimal_mod SHARED
    src/mod.c
)

set_target_properties(dttr_minimal_mod PROPERTIES
    PREFIX ""
    OUTPUT_NAME "minimal-mod"
)

target_link_libraries(dttr_minimal_mod PRIVATE DTTR::sdk)
