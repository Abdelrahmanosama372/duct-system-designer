cmake_minimum_required(VERSION 3.24)

project(DuctSystemDesigner LANGUAGES CXX)

option(DSD_BUILD_AUTOCAD "Build the AutoCAD ObjectARX 2027 application" ${WIN32})
if(DSD_BUILD_AUTOCAD)
    if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "DSD_BUILD_AUTOCAD requires Windows, MSVC, and an x64 generator.")
    endif()
    # AutoCAD's release CRT must match across the plugin, core, and project tests.
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
endif()

add_subdirectory(src/core)
add_subdirectory(src/application)

if(DSD_BUILD_AUTOCAD)
    add_subdirectory(src/adapters/autocad)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
