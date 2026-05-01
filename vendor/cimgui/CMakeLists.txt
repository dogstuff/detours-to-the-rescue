find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_STATIC_LIBS OFF)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)

add_library(cimgui_static STATIC
    cimgui.cpp
    cimgui_impl.cpp
    cimgui_impl_sdlgpu3.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_sdlgpu3.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(cimgui_static PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
    ${SDL3_INCLUDE_DIRS}
)

target_compile_options(cimgui_static PRIVATE ${SDL3_CFLAGS_OTHER})
target_link_libraries(cimgui_static PRIVATE glad)

target_compile_definitions(cimgui_static PUBLIC
    CIMGUI_USE_SDL3
    CIMGUI_USE_OPENGL3
    CIMGUI_NO_EXPORT
)

target_compile_definitions(cimgui_static INTERFACE
    CIMGUI_DEFINE_ENUMS_AND_STRUCTS
)

target_compile_definitions(cimgui_static PRIVATE
    IMGUI_IMPL_API=extern\ \"C\"
    IMGUI_IMPL_OPENGL_LOADER_CUSTOM
)

set_source_files_properties(
    imgui/backends/imgui_impl_opengl3.cpp
    PROPERTIES COMPILE_OPTIONS "-include;glad/gl.h"
)
