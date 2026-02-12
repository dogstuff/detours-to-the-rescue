add_library(common STATIC src/interop.c src/crashdump.c)
target_include_directories(common PUBLIC include)
target_link_libraries(common PRIVATE sds log dbghelp)
