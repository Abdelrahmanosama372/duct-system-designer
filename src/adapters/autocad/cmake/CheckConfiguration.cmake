if(UI_CONFIG STREQUAL "Debug")
    message(FATAL_ERROR "Use Release or RelWithDebInfo with AutoCAD and the release ObjectARX SDK libraries.")
endif()
