add_subdirectory(core)
add_subdirectory(application)

if(DSD_BUILD_AUTOCAD)
    add_subdirectory(adapters/autocad)
endif()
