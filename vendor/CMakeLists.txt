add_library(log STATIC log/log.c)
target_include_directories(log PUBLIC log)

add_library(klib INTERFACE)
target_include_directories(klib INTERFACE klib)

add_library(xxhash STATIC xxhash/xxhash.c)
target_include_directories(xxhash PUBLIC xxhash)
target_compile_definitions(xxhash INTERFACE XXH_STATIC_LINKING_ONLY)

add_library(sds STATIC sds/sds.c)
target_include_directories(sds PUBLIC sds)

add_library(yyjson STATIC yyjson/yyjson.c)
target_include_directories(yyjson PUBLIC yyjson)

# Use Zydis only as a lightweight decoder dependency for hook prologue relocation.
set(ZYDIS_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_MAN OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_DOXYGEN OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_ENCODER OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_FORMATTER OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_AVX512 OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_KNC OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_SEGMENT OFF CACHE BOOL "" FORCE)
set(ZYDIS_MINIMAL_MODE ON CACHE BOOL "" FORCE)
add_subdirectory(zydis)

add_subdirectory(glad)
