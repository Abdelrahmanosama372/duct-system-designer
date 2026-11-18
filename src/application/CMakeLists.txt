# Application sources compile into the host module; there is no application library.
function(dsd_add_application_sources target)
    target_sources(${target} PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/application.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_processing.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_properties.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_analyzer.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_registration.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_sizing_configuration.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_topology_analyzer.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/duct_system_builder.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_geometry_generator.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_discovery.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_segmentor.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_equipment.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/network_topology.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/visualization_rectangle_factory.cpp"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/visualization_text_factory.cpp"
    )
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/include"
    )
    target_link_libraries(${target} PRIVATE dsd::core)
endfunction()
