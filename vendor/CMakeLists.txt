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
