include(CMakeParseArguments)

macro(dttr_find_sdl3)
  find_package(PkgConfig REQUIRED)
  if(NOT TARGET PkgConfig::SDL3)
    set(PKG_CONFIG_USE_STATIC_LIBS OFF)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
  endif()
endmacro()

macro(dttr_find_sdl3_mixer)
  find_package(PkgConfig REQUIRED)
  if(NOT TARGET PkgConfig::SDL3_MIXER)
    set(PKG_CONFIG_USE_STATIC_LIBS OFF)
    pkg_check_modules(SDL3_MIXER REQUIRED IMPORTED_TARGET sdl3-mixer)
  endif()
endmacro()

function(dttr_configure_runtime_dependencies)
  set(DTTR_TOOLCHAIN_TARGET "i686-w64-mingw32")
  set(DTTR_SDL3_RUNTIME_DIR
        "${CMAKE_SOURCE_DIR}/.toolchain/sdl3/${DTTR_TOOLCHAIN_TARGET}/bin"
    )
  set(DTTR_SDL3_MIXER_RUNTIME_DIR
        "${CMAKE_SOURCE_DIR}/.toolchain/sdl3_mixer/${DTTR_TOOLCHAIN_TARGET}/bin"
    )
  set(DTTR_SDL3_RUNTIME_DLL "${DTTR_SDL3_RUNTIME_DIR}/SDL3.dll")

  set(DTTR_SDL_RUNTIME_DLLS
        "${DTTR_SDL3_RUNTIME_DLL}"
        "${DTTR_SDL3_MIXER_RUNTIME_DIR}/SDL3_mixer.dll"
        PARENT_SCOPE
    )
  set(DTTR_SDL3_RUNTIME_DLL "${DTTR_SDL3_RUNTIME_DLL}" PARENT_SCOPE)
endfunction()

function(dttr_configure_ffmpeg)
  set(DTTR_FFMPEG_DIR "${CMAKE_SOURCE_DIR}/.toolchain/ffmpeg")
  set(DTTR_FFMPEG_LIBS avformat avcodec avutil swscale swresample)
  set(DTTR_FFMPEG_TARGETS ${DTTR_FFMPEG_LIBS})
  list(TRANSFORM DTTR_FFMPEG_TARGETS PREPEND "ffmpeg_")

  file(GLOB DTTR_FFMPEG_RUNTIME_DLLS "${DTTR_FFMPEG_DIR}/bin/*.dll")
  list(SORT DTTR_FFMPEG_RUNTIME_DLLS)
  if(NOT DTTR_FFMPEG_RUNTIME_DLLS)
    message(FATAL_ERROR
            "No FFmpeg runtime DLLs found in ${DTTR_FFMPEG_DIR}/bin; run the toolchain/bootstrap setup first"
        )
  endif()

  foreach(DTTR_FFMPEG_LIB DTTR_FFMPEG_TARGET IN ZIP_LISTS
        DTTR_FFMPEG_LIBS
        DTTR_FFMPEG_TARGETS
    )
    if(NOT EXISTS "${DTTR_FFMPEG_DIR}/lib/lib${DTTR_FFMPEG_LIB}.dll.a")
      message(FATAL_ERROR
                "Missing FFmpeg import library: ${DTTR_FFMPEG_DIR}/lib/lib${DTTR_FFMPEG_LIB}.dll.a; run the toolchain/bootstrap setup first"
            )
    endif()

    if(NOT TARGET ${DTTR_FFMPEG_TARGET})
      add_library(${DTTR_FFMPEG_TARGET} SHARED IMPORTED)
      set_target_properties(${DTTR_FFMPEG_TARGET} PROPERTIES
                IMPORTED_IMPLIB "${DTTR_FFMPEG_DIR}/lib/lib${DTTR_FFMPEG_LIB}.dll.a"
            )
    endif()
  endforeach()

  set(DTTR_FFMPEG_DIR "${DTTR_FFMPEG_DIR}" PARENT_SCOPE)
  set(DTTR_FFMPEG_TARGETS ${DTTR_FFMPEG_TARGETS} PARENT_SCOPE)
  set(DTTR_FFMPEG_RUNTIME_DLLS ${DTTR_FFMPEG_RUNTIME_DLLS} PARENT_SCOPE)
endfunction()

function(dttr_use_windows_static_runtime target)
  cmake_parse_arguments(DTTR_TARGET "WIN32_RELEASE" "" "" ${ARGN})

  target_link_options("${target}" PRIVATE -static)
  if(DTTR_TARGET_WIN32_RELEASE)
    target_link_options("${target}" PRIVATE $<$<CONFIG:Release>:-mwindows>)
  endif()
endfunction()

function(dttr_set_target_version target)
  target_compile_definitions("${target}" PRIVATE DTTR_VERSION="${DTTR_VERSION}")
endfunction()

function(dttr_copy_runtime_files target output_dir)
  cmake_parse_arguments(DTTR_COPY "" "" "FILES" ${ARGN})

  add_custom_command(TARGET "${target}" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DTTR_COPY_FILES}
            "${output_dir}"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()

function(dttr_copy_modding_sdk_distribution target output_dir)
  add_dependencies("${target}" dttr_sdk_bundle_header)
  get_property(DTTR_GENERATED_SDK_BUNDLE_HEADER GLOBAL PROPERTY DTTR_SDK_BUNDLE_HEADER)
  if(NOT DTTR_GENERATED_SDK_BUNDLE_HEADER)
    set(DTTR_GENERATED_SDK_BUNDLE_HEADER "${DTTR_SDK_BUNDLE_HEADER}")
  endif()

  add_custom_command(TARGET "${target}" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf
            "${output_dir}/include"
            "${output_dir}/lib"
            "${output_dir}/cmake"
            "${output_dir}/scripts"
            "${output_dir}/blueprints"
            "${output_dir}/docs"
            "${output_dir}/sdk"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${output_dir}/mods"
            "${output_dir}/sdk/include"
            "${output_dir}/sdk/lib"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DTTR_GENERATED_SDK_BUNDLE_HEADER}"
            "${output_dir}/sdk/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_LINKER_FILE:dttr_sidecar>
            "${output_dir}/sdk/lib"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/modules/sdk/dttrsdk-config.cmake"
            "${output_dir}/sdk"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()
