set(DTTR_CONFIG_WIN32_LIBS
    kernel32
    user32
)

add_executable(dttr_config_exe
    src/main.c
)

dttr_use_windows_static_runtime(dttr_config_exe WIN32_RELEASE)

add_library(dttr_config SHARED
    src/gui.c
    src/gui_dialogs.c
    src/gui_mod_load_path.c
    src/gui_mod_configs.c
    src/gui_state.c
    src/gui_tab_audio.c
    src/gui_tab_gamepad.c
    src/gui_tab_general.c
    src/gui_tab_graphics.c
    src/gui_tab_modding.c
    src/gui_tabs.c
    src/gui_widgets.c
)

dttr_use_windows_static_runtime(dttr_config)

dttr_set_target_version(dttr_config)
target_compile_definitions(dttr_config PRIVATE
    $<$<BOOL:${DTTR_MODS_ENABLED}>:DTTR_MODS_ENABLED>
)
target_include_directories(dttr_config PRIVATE
    "${DTTR_SDK_GENERATED_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/modules/sdk/include"
)

target_link_libraries(dttr_config PRIVATE
    ${DTTR_CONFIG_WIN32_LIBS}
    opengl32
    glad
    common
)

target_link_libraries(dttr_config_exe PRIVATE
    ${DTTR_CONFIG_WIN32_LIBS}
)

set_target_properties(dttr_config_exe PROPERTIES
    OUTPUT_NAME "dttr-config"
)

set(DTTR_CONFIG_MODULES_DIR "$<TARGET_FILE_DIR:dttr_config_exe>/modules")
add_dependencies(dttr_config_exe dttr_config dttr_sidecar)
dttr_copy_runtime_files(dttr_config_exe "${DTTR_CONFIG_MODULES_DIR}"
    FILES
        $<TARGET_FILE:dttr_config>
        ${DTTR_SDL3_RUNTIME_DLL}
)

if(BUILD_TESTING)
    include("${CMAKE_CURRENT_LIST_DIR}/tests/tests.cmake")
endif()
