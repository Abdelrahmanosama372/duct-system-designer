add_library(dsd_core STATIC
    ductsystem/src/configs.cpp
    ductsystem/src/sheet_metal_component.cpp
    ductsystem/src/rectangular_models.cpp
    ductsystem/src/sheet_metal_node.cpp
    ductsystem/src/duct_fitting_tree.cpp
    ductsystem/src/duct_system.cpp
    ductsystem/src/visitors/sheet_metal_component_visitor.cpp
    ductsystem/src/visitors/area_calculator.cpp
    ductsystem/src/visitors/geometry_constructor.cpp
    geometry/src/primitives.cpp
    geometry/src/geometric_representation.cpp
    geometry/src/source_geometry_repository.cpp
    geometry/src/rectangular_geometry_builder.cpp
    geometry/src/placement_context_manager.cpp
    sizing/src/models.cpp
    sizing/src/duct_sizer_config.cpp
    sizing/src/duct_sizer.cpp
    sizing/src/equal_velocity.cpp
    sizing/src/equal_friction.cpp
)
add_library(dsd::core ALIAS dsd_core)

target_compile_features(dsd_core PUBLIC cxx_std_20)
set_target_properties(dsd_core PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(dsd_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sizing/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/ductsystem/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/geometry/include"
)
