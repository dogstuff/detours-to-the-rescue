add_executable(dttr src/main.c src/compat.c src/load.c)
target_include_directories(dttr PRIVATE include)
target_link_options(dttr PRIVATE -static)
target_link_libraries(dttr PRIVATE kernel32 psapi opengl32 sds log common)
