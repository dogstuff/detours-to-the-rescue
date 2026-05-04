set(DTTR_CONFIG_WIN32_LIBS
    kernel32
    user32
)

add_executable(dttr_config_exe
    src/main.c
)

target_link_options(dttr_config_exe PRIVATE
    -static
    $<$<CONFIG:Release>:-mwindows>
)

add_library(dttr_config SHARED
    src/gui.c
    src/gui_dialogs.c
    src/gui_state.c
    src/gui_tab_audio.c
    src/gui_tab_gamepad.c
    src/gui_tab_general.c
    src/gui_tab_graphics.c
    src/gui_tab_modding.c
    src/gui_tabs.c
    src/gui_widgets.c
)

target_link_options(dttr_config PRIVATE -static)

target_compile_definitions(dttr_config PRIVATE
    DTTR_VERSION="${DTTR_VERSION}"
    $<$<BOOL:${DTTR_MODDING}>:DTTR_MODDING_ENABLED>
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
add_custom_command(TARGET dttr_config_exe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DTTR_CONFIG_MODULES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:dttr_config>
        "$<TARGET_FILE_DIR:dttr_sidecar>/SDL3.dll"
        "${DTTR_CONFIG_MODULES_DIR}"
)
