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

add_library(dr_wav INTERFACE)
target_include_directories(dr_wav INTERFACE dr_libs)

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

# Keep PhysicsFS focused on the one archive type DttR needs for direct ISO loading.
set(PHYSFS_ARCHIVE_ZIP OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_7Z OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_GRP OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_WAD OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_CSM OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_HOG OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_MVL OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_QPAK OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_ROFS OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_SLB OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_ISO9660 ON CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_VDF OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_LECARCHIVES OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_POD OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(PHYSFS_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(physfs)

add_subdirectory(glad)

add_subdirectory(cimgui)
