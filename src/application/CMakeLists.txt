# Application sources compile into the host module; there is no application library.
function(dsd_add_application_sources target)
    target_sources(${target} PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/application.cpp"
    )
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/include"
    )
    target_link_libraries(${target} PRIVATE dsd::core)
endfunction()
