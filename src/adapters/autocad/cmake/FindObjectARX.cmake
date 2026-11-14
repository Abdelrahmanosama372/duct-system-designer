# ObjectARX 2027 (AutoCAD 2027, x64). The parent project may supply its own
# finder with the same ObjectARX::ObjectARX target and definition-file variable.
include(FindPackageHandleStandardArgs)
set(OBJECTARX_ROOT "$ENV{ARX}" CACHE PATH "ObjectARX 2027 SDK root")
find_path(OBJECTARX_INCLUDE_DIR adui.h PATHS "${OBJECTARX_ROOT}/inc" NO_DEFAULT_PATH)
find_path(OBJECTARX_X64_INCLUDE_DIR acadi.h PATHS "${OBJECTARX_ROOT}/inc-x64" NO_DEFAULT_PATH)
find_file(OBJECTARX_DEFINITION_FILE AcRxDefault.def PATHS "${OBJECTARX_ROOT}/inc" NO_DEFAULT_PATH)
set(_ObjectARX_required OBJECTARX_INCLUDE_DIR OBJECTARX_X64_INCLUDE_DIR OBJECTARX_DEFINITION_FILE)
set(_ObjectARX_libraries)
foreach(_library IN ITEMS rxapi acdb26 accore acad adui26 acui26 AcPal)
    find_library(OBJECTARX_${_library}_LIBRARY NAMES ${_library}
        PATHS "${OBJECTARX_ROOT}/lib-x64" NO_DEFAULT_PATH)
    list(APPEND _ObjectARX_required OBJECTARX_${_library}_LIBRARY)
    list(APPEND _ObjectARX_libraries "${OBJECTARX_${_library}_LIBRARY}")
endforeach()
find_package_handle_standard_args(ObjectARX REQUIRED_VARS ${_ObjectARX_required})
if(ObjectARX_FOUND AND NOT TARGET ObjectARX::ObjectARX)
    add_library(ObjectARX::ObjectARX INTERFACE IMPORTED)
    set_target_properties(ObjectARX::ObjectARX PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OBJECTARX_INCLUDE_DIR};${OBJECTARX_X64_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${_ObjectARX_libraries}")
endif()
