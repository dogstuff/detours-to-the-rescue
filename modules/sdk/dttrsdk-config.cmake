# The DttR modding SDK loaded from an unpacked DttR distribution (/sdk)

get_filename_component(_DTTRSDK_DIST_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(_DTTRSDK_SIDECAR_DLL "${_DTTRSDK_DIST_DIR}/modules/libdttr_sidecar.dll")
set(_DTTRSDK_SIDECAR_IMPLIB "${CMAKE_CURRENT_LIST_DIR}/lib/libdttr_sidecar.dll.a")
set(_DTTRSDK_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(_DTTRSDK_BUNDLE_HEADER "${_DTTRSDK_INCLUDE_DIR}/dttr_sdk.h")

foreach(_DTTRSDK_PATH IN ITEMS
    "${_DTTRSDK_SIDECAR_DLL}"
    "${_DTTRSDK_SIDECAR_IMPLIB}"
    "${_DTTRSDK_INCLUDE_DIR}"
    "${_DTTRSDK_BUNDLE_HEADER}"
)
    if(NOT EXISTS "${_DTTRSDK_PATH}")
        message(FATAL_ERROR "DttR SDK package is missing required path: ${_DTTRSDK_PATH}")
    endif()
endforeach()

function(_dttrsdk_expect_target_property target property expected)
    get_target_property(_DTTRSDK_ACTUAL "${target}" "${property}")
    if(NOT _DTTRSDK_ACTUAL STREQUAL "${expected}")
        message(FATAL_ERROR
            "${target} already exists with ${property}='${_DTTRSDK_ACTUAL}', "
            "but this DttR SDK package requires '${expected}'"
        )
    endif()
endfunction()

if(TARGET DTTR::sidecar)
    _dttrsdk_expect_target_property(DTTR::sidecar IMPORTED_LOCATION "${_DTTRSDK_SIDECAR_DLL}")
    _dttrsdk_expect_target_property(DTTR::sidecar IMPORTED_IMPLIB "${_DTTRSDK_SIDECAR_IMPLIB}")
    _dttrsdk_expect_target_property(DTTR::sidecar INTERFACE_INCLUDE_DIRECTORIES "${_DTTRSDK_INCLUDE_DIR}")
else()
    add_library(DTTR::sidecar SHARED IMPORTED)
    set_target_properties(DTTR::sidecar PROPERTIES
        IMPORTED_LOCATION "${_DTTRSDK_SIDECAR_DLL}"
        IMPORTED_IMPLIB "${_DTTRSDK_SIDECAR_IMPLIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${_DTTRSDK_INCLUDE_DIR}"
    )
endif()

if(TARGET DTTR::sdk)
    get_target_property(_DTTRSDK_SDK_ALIAS_TARGET DTTR::sdk ALIASED_TARGET)
    if(NOT _DTTRSDK_SDK_ALIAS_TARGET STREQUAL "DTTR::sidecar")
        message(FATAL_ERROR
            "DTTR::sdk already exists but is not an alias for DTTR::sidecar "
            "from this DttR SDK package"
        )
    endif()
else()
    add_library(DTTR::sdk ALIAS DTTR::sidecar)
endif()

unset(_DTTRSDK_DIST_DIR)
unset(_DTTRSDK_SIDECAR_DLL)
unset(_DTTRSDK_SIDECAR_IMPLIB)
unset(_DTTRSDK_INCLUDE_DIR)
unset(_DTTRSDK_BUNDLE_HEADER)
unset(_DTTRSDK_PATH)
unset(_DTTRSDK_SDK_ALIAS_TARGET)
