add_executable(dttr src/main.c)
target_include_directories(dttr PRIVATE include)
target_link_libraries(dttr PRIVATE kernel32 psapi opengl32 sds log common)
