add_library(dttr_watchdog SHARED
    src/watchdog.c
)

target_include_directories(dttr_watchdog PRIVATE
    "${CMAKE_SOURCE_DIR}/loader/include"
)

target_link_options(dttr_watchdog PRIVATE -static)

target_link_libraries(dttr_watchdog PRIVATE
    kernel32
    dbghelp
)
