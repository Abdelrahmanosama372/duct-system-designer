cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(DuctSystemDesignerAutocad VERSION 0.15.0 LANGUAGES CXX)
endif()

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This ObjectARX UI requires Windows, MSVC, and an x64 generator.")
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
# Support using this folder as the CMake entry point within the full checkout.
# Add core before enabling MFC so core remains independent of the UI framework.
if(NOT TARGET dsd::core)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../core"
                     "${CMAKE_CURRENT_BINARY_DIR}/core")
endif()
if(NOT COMMAND dsd_add_application_sources)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../application"
                     "${CMAKE_CURRENT_BINARY_DIR}/application")
endif()
set(CMAKE_MFC_FLAG 2)
# Use the parent project's FindObjectARX.cmake when supplied; otherwise use
# the adapter's bundled fallback for standalone builds.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(NOT TARGET ObjectARX::ObjectARX)
    find_package(ObjectARX REQUIRED)
endif()
set(DUCT_UI_OUTPUT_NAME "TestARX15" CACHE STRING "Output ARX filename without extension")
if(NOT OBJECTARX_DEFINITION_FILE)
    set(OBJECTARX_DEFINITION_FILE "${OBJECTARX_ROOT}/inc/AcRxDefault.def")
endif()
if(NOT EXISTS "${OBJECTARX_DEFINITION_FILE}")
    message(FATAL_ERROR "Set OBJECTARX_DEFINITION_FILE to the SDK's AcRxDefault.def.")
endif()

add_library(DuctSystemDesignerUi MODULE
    src/ui/Designer.cpp
    src/Module.cpp
    src/commands/UiCommands.cpp
    src/database/NetworkInteraction.cpp
    src/ui/PalettePage.cpp
    src/ui/controls/UiButton.cpp
    src/ui/controls/UiComboBox.cpp
    src/ui/controls/UiEdit.cpp
    src/ui/controls/UiHeading.cpp
    src/ui/controls/UiScrollRail.cpp
    src/ui/controls/UiSeparator.cpp
    src/ui/controls/UiTabs.cpp
    src/ui/layout/ScrollPage.cpp
    src/ui/layout/TabbedContent.cpp
    src/ui/model/NetworkSession.cpp
    src/ui/model/NetworkActions.cpp
    src/ui/model/ApplicationSession.cpp
    src/ui/model/ProjectInformation.cpp
    src/ui/panels/DrawingGenerationPanel.cpp
    src/ui/panels/NetworkPanel.cpp
    src/ui/panels/ProjectInformationPanel.cpp
    src/ui/panels/PropertiesPanel.cpp
    src/ui/panels/ReportsPanel.cpp
)
add_library(DuctUi::Designer ALIAS DuctSystemDesignerUi)
dsd_add_application_sources(DuctSystemDesignerUi)
target_compile_features(DuctSystemDesignerUi PRIVATE cxx_std_20)
target_compile_definitions(DuctSystemDesignerUi PRIVATE
    _AFXEXT _AFXDLL UNICODE _UNICODE WIN32 _WINDOWS
    _WIN32_WINNT=0x0A00 _WIN32_IE=0x0601)
target_compile_options(DuctSystemDesignerUi PRIVATE /EHsc /utf-8)
target_include_directories(DuctSystemDesignerUi
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
set_target_properties(DuctSystemDesignerUi PROPERTIES
    PREFIX "" SUFFIX ".arx" OUTPUT_NAME "${DUCT_UI_OUTPUT_NAME}"
    CXX_EXTENSIONS OFF
    MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
target_link_libraries(DuctSystemDesignerUi PRIVATE ObjectARX::ObjectARX)
target_link_libraries(DuctSystemDesignerUi PRIVATE gdiplus uxtheme comctl32)
target_link_options(DuctSystemDesignerUi PRIVATE
    "/DEF:${OBJECTARX_DEFINITION_FILE}" /INCREMENTAL:NO)
# AutoCAD and these SDK libraries use the release MFC/CRT. Reject Debug explicitly.
add_custom_command(TARGET DuctSystemDesignerUi PRE_BUILD
    COMMAND "${CMAKE_COMMAND}" -DUI_CONFIG=$<CONFIG>
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckConfiguration.cmake")
add_custom_command(TARGET DuctSystemDesignerUi POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts"
            "$<TARGET_FILE_DIR:DuctSystemDesignerUi>/fonts"
    COMMENT "Copying privately loaded Inter fonts beside the ARX")

option(DUCT_UI_BUILD_CHECKS "Build native MFC regression checks (no AutoCAD required to run)" OFF)
if(DUCT_UI_BUILD_CHECKS)
    include(CTest)
    add_executable(DuctUiNativeChecks WIN32
        tests/NativeUiChecks.cpp
    src/ui/controls/UiButton.cpp
    src/ui/controls/UiComboBox.cpp
    src/ui/controls/UiEdit.cpp
    src/ui/controls/UiHeading.cpp
    src/ui/controls/UiScrollRail.cpp
    src/ui/controls/UiSeparator.cpp
    src/ui/controls/UiTabs.cpp
    src/ui/layout/ScrollPage.cpp
    src/ui/layout/TabbedContent.cpp
    src/ui/model/NetworkSession.cpp
    src/ui/model/NetworkActions.cpp
    src/ui/model/ApplicationSession.cpp
    src/ui/model/ProjectInformation.cpp
    src/ui/panels/DrawingGenerationPanel.cpp
    src/ui/panels/NetworkPanel.cpp
    src/ui/panels/ProjectInformationPanel.cpp
    src/ui/panels/PropertiesPanel.cpp
    src/ui/panels/ReportsPanel.cpp
    )
    target_compile_features(DuctUiNativeChecks PRIVATE cxx_std_20)
    dsd_add_application_sources(DuctUiNativeChecks)
    target_compile_definitions(DuctUiNativeChecks PRIVATE DUCT_UI_NATIVE_TESTS _AFXDLL UNICODE _UNICODE _WIN32_WINNT=0x0A00 _WIN32_IE=0x0601)
    target_compile_options(DuctUiNativeChecks PRIVATE /EHsc /utf-8)
    target_include_directories(DuctUiNativeChecks PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_target_properties(DuctUiNativeChecks PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
    target_link_libraries(DuctUiNativeChecks PRIVATE gdiplus uxtheme comctl32)
    add_custom_command(TARGET DuctUiNativeChecks POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts" "$<TARGET_FILE_DIR:DuctUiNativeChecks>/fonts")
    add_test(NAME NativeUiChecks COMMAND DuctUiNativeChecks)
    set_tests_properties(NativeUiChecks PROPERTIES WORKING_DIRECTORY "$<TARGET_FILE_DIR:DuctUiNativeChecks>" TIMEOUT 90)
endif()
