find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(sizing_tests
    unit/sizing/test_models.cpp
    unit/sizing/test_duct_sizer_config.cpp
    unit/sizing/test_duct_sizer.cpp
    unit/sizing/test_equal_velocity.cpp
    unit/sizing/test_equal_friction.cpp
)
target_link_libraries(sizing_tests PRIVATE dsd::core GTest::gtest_main)
set_target_properties(sizing_tests PROPERTIES CXX_EXTENSIONS OFF)
gtest_discover_tests(sizing_tests NO_PRETTY_VALUES)

add_executable(ductsystem_tests
    unit/ductsystem/test_component_models.cpp
    unit/ductsystem/test_rectangular_duct.cpp
    unit/ductsystem/test_rectangular_fittings.cpp
    unit/ductsystem/test_configs.cpp
    unit/ductsystem/test_sheet_metal_node.cpp
    unit/ductsystem/test_duct_fitting_tree.cpp
    unit/ductsystem/test_duct_system.cpp
    unit/ductsystem/visitors/test_sheet_metal_component_visitor.cpp
    unit/ductsystem/visitors/test_area_calculator.cpp
)
target_link_libraries(ductsystem_tests PRIVATE dsd::core GTest::gtest_main)
set_target_properties(ductsystem_tests PROPERTIES CXX_EXTENSIONS OFF)
gtest_discover_tests(ductsystem_tests NO_PRETTY_VALUES)

add_executable(geometry_tests
    unit/geometry/test_primitives.cpp
    unit/geometry/test_rectangular_geometry_builder.cpp
    unit/geometry/test_placement.cpp
    integration/test_rectangular_geometry_pipeline.cpp
)
target_link_libraries(geometry_tests PRIVATE dsd::core GTest::gtest_main)
set_target_properties(geometry_tests PROPERTIES CXX_EXTENSIONS OFF)
gtest_discover_tests(geometry_tests NO_PRETTY_VALUES)

add_executable(application_tests
    application/test_application.cpp
    application/test_network_discovery.cpp
    application/test_network_segmentor.cpp
    application/test_visualization_rectangle_factory.cpp
    application/test_network_processing.cpp
)
dsd_add_application_sources(application_tests)
target_link_libraries(application_tests PRIVATE GTest::gtest_main)
set_target_properties(application_tests PROPERTIES CXX_EXTENSIONS OFF)
gtest_discover_tests(application_tests NO_PRETTY_VALUES)
