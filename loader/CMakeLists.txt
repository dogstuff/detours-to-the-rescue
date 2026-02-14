add_executable(dttr src/main.c src/nt_process.c src/compat_shims_overrides.c src/load.c src/watchdog.c)
target_include_directories(dttr PRIVATE include)
target_link_options(dttr PRIVATE -static $<$<CONFIG:Release>:-mwindows>)
target_link_libraries(dttr PRIVATE kernel32 psapi opengl32 dbghelp sds log common)
